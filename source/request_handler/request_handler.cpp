#include "request_handler.hpp"
#include <format>
#include <thread>
#include <mutex>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include <HttpStatusCodes_C++11.h>
#include <imgui.h>

#include "../../cmake-build-release-visual-studio/_deps/curlpp-src/include/curlpp/cURLpp.hpp"
#include "logger/logger.hpp"

static char token[100] = DEFAULT_TOKEN;
enum { token_buffer_size = sizeof(token) / sizeof(char) };
static std::mutex mutex;

void request_handler::draw_token_input()
{	ImGui::Text("Bot Token");
	ImGui::SameLine();
	mutex.lock();
	ImGui::InputText("##token input", token, token_buffer_size);
	mutex.unlock();
}

nlohmann::json request_handler::run
(	std::string path,
	std::map<std::string, std::string> options,
	std::optional<nlohmann::json> body
)
{	struct response { int code; nlohmann::json data; };
	auto request =
		[path, options, body]() -> response
		{	std::ostringstream output;
			std::optional<std::istringstream> input;
			if(body)
				input = std::istringstream(body->dump());
			curlpp::Easy request;
			request.setOpt<curlpp::OptionTrait<curl_blob*, CURLOPT_CAINFO_BLOB>>(&ca_certificates);
			std::stringstream link;
			link << "https://discord.com/api/v10/" << path;
			if(!options.empty())
			{	link << '?';
				for(auto iterator = options.begin(); iterator != options.end(); ++iterator)
				{	if(iterator != options.begin())
						link << '&';
					link
						<< curlpp::escape(iterator->first)
						<< '='
						<< curlpp::escape(iterator->second);
				}
			}
			request.setOpt<curlpp::options::Url>(link.str());
			request.setOpt<curlpp::options::WriteStream>(&output);
			if(input)
				request.setOpt<curlpp::options::ReadStream>(&input.value());
			mutex.lock();
			request.setOpt<curlpp::options::HttpHeader>({ std::format("Authorization: Bot {}", token) });
			mutex.unlock();
			request.perform();
			return
				{	.code = (int)curlpp::infos::ResponseCode::get(request),
					.data = nlohmann::json::parse(output.str())
				};
		};
	logger::message(std::format("Requesting path : '{}'", path));
	response response = request();
	if(response.code == HttpStatus::toInt(HttpStatus::Code::TooManyRequests))
	{	float wait = response.data["retry_after"];
		logger::message(std::format("Rate limit exceeded, retrying after recommended {} seconds.", wait));
		std::this_thread::sleep_for(std::chrono::milliseconds((int)std::round(wait * 1000)));
		logger::message("Retrying.");
		response = request();
	}
	if(HttpStatus::isSuccessful(response.code))
		return response.data;
	else
		throw
			std::runtime_error
			(	std::format
				(	"Request failed with error {} ({}) :\n{}",
					response.code,
					HttpStatus::reasonPhrase(response.code),
					response.data.dump()
				)
			);
}

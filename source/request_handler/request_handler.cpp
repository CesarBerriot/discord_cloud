#include "request_handler.hpp"
#include <format>
#include <thread>
#include <mutex>
#include <fstream>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Infos.hpp>
#include <HttpStatusCodes_C++11.h>
#include <imgui.h>

#include "../../cmake-build-release-visual-studio/_deps/curlpp-src/include/curlpp/cURLpp.hpp"
#include "helpers/helpers.hpp"
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
	std::optional<nlohmann::json> body,
	std::map<std::string, std::vector<byte>> files,
	std::optional<std::string> method
)
{	struct response { int code; nlohmann::json data; };
	auto request =
		[path, options, body, files, method]() -> response
		{	std::vector<std::function<void()>> cleanup_procedures;
			std::ostringstream output;
			curlpp::Easy request;
			request.setOpt<curlpp::OptionTrait<curl_blob*, CURLOPT_CAINFO_BLOB>>(&ca_certificates);
			if(method)
				request.setOpt<curlpp::options::CustomRequest>(*method);
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
			if(!files.empty())
			{	curlpp::Forms form_parts;
				if(body)
					form_parts.push_back(new curlpp::FormParts::Content("payload_json", body->dump()));
				std::filesystem::path temporary_directory = helpers::create_temporary_directory();
				cleanup_procedures.push_back([temporary_directory] { std::filesystem::remove_all(temporary_directory); });
				for(std::pair pair : files)
				{	std::filesystem::path file_path = temporary_directory / pair.first;
					std::ofstream(file_path, std::ios::binary)
					.write((char*)pair.second.data(), pair.second.size());
					form_parts.push_back(new curlpp::FormParts::File(pair.first, file_path.string()));
				}
				request.setOpt<curlpp::options::HttpPost>(form_parts);
			}
			else
				if(body)
				{	std::string dump = body->dump();
					request.setOpt<curlpp::options::PostFields>(dump);
					request.setOpt<curlpp::options::PostFieldSize>(dump.length());
				}
			mutex.lock();
			request.setOpt<curlpp::options::HttpHeader>
			(	{	std::format("Authorization: Bot {}", token),
					std::format("Content-Type: {}", files.empty() ? "application/json" : "multipart/form-data")
				}
			);
			mutex.unlock();
			request.perform();
			for(std::function procedure : cleanup_procedures)
				procedure();
			std::string output_dump = output.str();
			return
				{	.code = (int)curlpp::infos::ResponseCode::get(request),
					.data =
						output_dump.empty()
						? nlohmann::json()
						: nlohmann::json::parse(output.str())
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

std::vector<byte> request_handler::download(std::string link)
{	std::vector<byte> result;
	curlpp::Easy request;
	request.setOpt<curlpp::OptionTrait<curl_blob*, CURLOPT_CAINFO_BLOB>>(&ca_certificates);
	request.setOpt<curlpp::options::Url>(link);
	request.setOpt<curlpp::Options::WriteFunction>
	(	[&result](char * data, size_t, size_t size) -> size_t
		{	result.insert(result.end(), (byte*)data, (byte*)data + size);
			return size;
		}
	);
	request.perform();
	int response_code = curlpp::infos::ResponseCode::get(request);
	if(!HttpStatus::isSuccessful(response_code))
		throw
			std::runtime_error
			(	std::format
				(	"Download failed with code {} : '{}'",
					response_code,
					HttpStatus::reasonPhrase(response_code)
				)
			);
	else
		return result;
}

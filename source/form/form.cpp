#include "form.hpp"
#include <memory>
#include <dial/form_base.hpp>
#include <imgui.h>
#include "channel_selector/channel_selector.hpp"
#include "logger/logger.hpp"
#include "request_handler/request_handler.hpp"
#include "task/task.hpp"
#include "file_interface/file_interface.hpp"

class form : dial::form_base
{	public:
		form();
	private:
		std::string get_name() override { return "master"; }
		void draw_contents() override;
		void process_log();
};

form::form()
{	task::is_running = [this] { return is_running_asynchronous_task(); };
	task::is_task_thread = [this] { return is_asynchronous_task_thread(); };
	task::run = [this](std::function<void()> procedure) { run_asynchronous_task(procedure); };
}

void form::draw_contents()
{	process_log();
	request_handler::draw_token_input();
	if(channel_selector::draw())
		file_interface::draw();
	else
		file_interface::clear();
}

void form::process_log()
{	for(std::pair pair : logger::pop())
		log((log_level::type)pair.first, pair.second);
}

void instantiate_form()
{	static auto instance = std::make_unique<form>();
}

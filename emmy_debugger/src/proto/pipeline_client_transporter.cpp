/*
* Copyright (c) 2019. tangzx(love.tangzx@qq.com)
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "emmy_debugger/proto/pipeline_client_transporter.h"

static void echo_alloc(uv_handle_t* handle,
                       size_t suggested_size,
                       uv_buf_t* buf) {
	buf->base = static_cast<char*>(malloc(suggested_size));
	buf->len = suggested_size;
}

static void after_read(uv_stream_t* handle,
                       ssize_t nread,
                       const uv_buf_t* buf) {
	auto p = static_cast<Transporter*>(handle->data);
	p->OnAfterRead(handle, nread, buf);
}

void onPipeConnectionCB(uv_connect_t* req, int status) {
	const auto t = (PipelineClientTransporter*)req->data;
	t->OnPipeConnection(req, status);
}

PipelineClientTransporter::PipelineClientTransporter(): Transporter(false) {
}

PipelineClientTransporter::~PipelineClientTransporter() {

}

int PipelineClientTransporter::Stop() {
	Transporter::Stop();
	if (IsConnected()) {
		uv_read_stop((uv_stream_t*)&uvClient);
		uv_close((uv_handle_t*)&uvClient, nullptr);
	}
	return 0;
}

bool PipelineClientTransporter::Connect(const std::string& name, std::string& err) {
	std::string fullName;
#ifdef _WIN32
	{
		fullName = "\\\\.\\pipe\\emmylua-";
		fullName.append(name);
	}
#else
	{
		char tmp[2048];
		size_t len = sizeof(tmp);
		uv_os_tmpdir(tmp, &len);
		fullName = tmp;
		fullName.append("/");
		fullName.append(name);
	}
#endif

	uvClient.data = this;
	const auto req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
	req->data = this;
	uv_pipe_init(loop, &uvClient, 0);
	uv_pipe_connect(req, &uvClient, fullName.c_str(), onPipeConnectionCB);
	StartEventLoop();

	std::unique_lock<std::mutex> lock(mutex);
	cv.wait(lock);
	return IsConnected();
}

void PipelineClientTransporter::Send(int cmd, const char* data, size_t len) {
	Transporter::Send((uv_stream_t*)&uvClient, cmd, data, len);
}

void PipelineClientTransporter::OnPipeConnection(uv_connect_t* pipe, int status) {
	if (status < 0) {
		Stop();
		OnConnect(false);
	}
	else {
		OnConnect(true);
		uv_read_start((uv_stream_t*)&uvClient, echo_alloc, after_read);
	}
	cv.notify_all();
}

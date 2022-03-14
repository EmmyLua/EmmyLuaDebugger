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
#include "emmy_debugger/proto/pipeline_server_transporter.h"

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

void onPipeConnectionCB(uv_stream_t* pipe, int status) {
	const auto t = (PipelineServerTransporter*)pipe->data;
	t->OnPipeConnection(pipe, status);
}

PipelineServerTransporter::PipelineServerTransporter():
	Transporter(true),
	uvClient(nullptr) {
}

PipelineServerTransporter::~PipelineServerTransporter() {

}

bool PipelineServerTransporter::pipe(const std::string& name, std::string& err) {
	loop = uv_default_loop();
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
        uv_fs_t req;
        uv_fs_unlink(nullptr, &req, fullName.c_str(), nullptr);
        uv_fs_req_cleanup(&req);
    }
#endif
	uvServer.data = this;
	uv_pipe_init(loop, &uvServer, 0);
	int r = uv_pipe_bind(&uvServer, fullName.c_str());
	if (r) {
		err = uv_err_name(r);
		return false;
	}
	r = uv_listen((uv_stream_t*)&uvServer, 128, onPipeConnectionCB);
	if (r) {
		err = uv_err_name(r);
		return false;
	}
	StartEventLoop();
	return true;
}

int PipelineServerTransporter::Stop() {
	Transporter::Stop();
	uv_close((uv_handle_t*)&uvServer, nullptr);
	if (uvClient) {
		uv_read_stop((uv_stream_t*)&uvClient);
		uv_close((uv_handle_t*)uvClient, nullptr);
	}
	return 0;
}

void PipelineServerTransporter::Send(int cmd, const char* data, size_t len) {
	Transporter::Send((uv_stream_t*)uvClient, cmd, data, len);
}

void PipelineServerTransporter::OnPipeConnection(uv_stream_t* pipe, int status) {
	if (status < 0) {
		Stop();
	}
	else {
        // todo: close prev uvClient
		uvClient = (uv_pipe_t*)malloc(sizeof(uv_pipe_t));
		uv_pipe_init(loop, uvClient, 0);
		uvClient->data = this;
		const int r = uv_accept((uv_stream_t*)&uvServer, (uv_stream_t*)uvClient);
		assert(r == 0);
		OnConnect(true);
		uv_read_start((uv_stream_t*)uvClient, echo_alloc, after_read);
	}
}

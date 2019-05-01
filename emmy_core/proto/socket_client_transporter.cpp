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
#include "socket_client_transporter.h"
#include <condition_variable>

void OnConnectCB(uv_connect_t* req, int status) {
    const auto thiz = (SocketClientTransporter*)req->data;
    thiz->OnConnection(req, status);
}

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

SocketClientTransporter::SocketClientTransporter():
    Transporter(false),
	uvClient({}),
    connect_req({}) {
}

SocketClientTransporter::~SocketClientTransporter() {
}

int SocketClientTransporter::Stop() {
    Transporter::Stop();
	if (IsConnected()) {
		uv_read_stop((uv_stream_t*)&uvClient);
		uv_close((uv_handle_t*)&uvClient, nullptr);
	}
    cv.notify_all();
    return 0;
}

bool SocketClientTransporter::Connect(const std::string& host, int port) {
	sockaddr_in addr{};
	uvClient.data = this;
	uv_tcp_init(loop, &uvClient);
	uv_ip4_addr(host.c_str(), port, &addr);
    connect_req.data = this;
	const int r = uv_tcp_connect(&connect_req, &uvClient, reinterpret_cast<const struct sockaddr*>(&addr), OnConnectCB);
	if (r) {
		fprintf(stderr, "Connect error %s\n", uv_strerror(r));
		return false;
	}
    StartEventLoop();
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock);
	return IsConnected();
}

void SocketClientTransporter::OnConnection(uv_connect_t *req, int status) {
    if (status >= 0) {
        OnConnect();
        uv_read_start((uv_stream_t*)&uvClient, echo_alloc, after_read);
    }
    else {
        Stop();
    }
    cv.notify_all();
}

void SocketClientTransporter::Send(int cmd, const char *data, size_t len) {
    Transporter::Send((uv_stream_t*)&uvClient, cmd, data, len);
}

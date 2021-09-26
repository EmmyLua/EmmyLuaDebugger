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
#include <cassert>
#include "emmy_debugger/proto/socket_server_transporter.h"

static void on_new_connection(uv_stream_t* server, int status) {
	auto p = reinterpret_cast<SocketServerTransporter*>(server->data);
	p->OnNewConnection(server, status);
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
	auto p = static_cast<SocketServerTransporter*>(handle->data);
	p->OnAfterRead(handle, nread, buf);
}

////////////////////////////////////////////////////////////////////////////////
// tcp server

SocketServerTransporter::SocketServerTransporter() : Transporter(true), uvServer() {
	uvClient = nullptr;
}

SocketServerTransporter::~SocketServerTransporter() {
}

bool SocketServerTransporter::Listen(const std::string& host, int port, std::string& err) {
	sockaddr_in addr{};
	uvServer.data = this;
	uv_tcp_init(loop, &uvServer);
	uv_ip4_addr(host.c_str(), port, &addr);
	uv_tcp_bind(&uvServer, reinterpret_cast<const struct sockaddr*>(&addr), 0);
	const int r = uv_listen(reinterpret_cast<uv_stream_t*>(&uvServer), SOMAXCONN, on_new_connection);
	if (r) {
		err = uv_strerror(r);
		return false;
	}
	StartEventLoop();
	return true;
}

void SocketServerTransporter::Send(const char* data, size_t len)
{
	Transporter::Send((uv_stream_t*)uvClient, data, len);
}

int SocketServerTransporter::Stop() {
	uv_close((uv_handle_t*)&uvServer, nullptr);
	if (uvClient) {
		uv_read_stop(uvClient);
		uv_close((uv_handle_t*)uvClient, nullptr);
	}
	Transporter::Stop();
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
// new connection & read

void SocketServerTransporter::OnNewConnection(uv_stream_t* server, int status) {
	// todo: free prev client
	uvClient = static_cast<uv_stream_t*>(malloc(sizeof(uv_tcp_t)));
	uvClient->data = this;

	int r = uv_tcp_init(loop, reinterpret_cast<uv_tcp_t*>(uvClient));
	assert(r == 0);

	r = uv_accept(server, uvClient);
	assert(r == 0);

	r = uv_read_start(uvClient, echo_alloc, after_read);
	assert(r == 0);

	OnConnect(true);
}

////////////////////////////////////////////////////////////////////////////////
// send data

void SocketServerTransporter::Send(int cmd, const char* data, size_t len) {
	Transporter::Send((uv_stream_t*)uvClient, cmd, data, len);
}

void SocketServerTransporter::OnDisconnect() {
	Transporter::OnDisconnect();
    
    uv_read_stop((uv_stream_t*)uvClient);
    // todo: free uvClient
    uvClient = nullptr;
}

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
#include "emmy_debugger/transporter/transporter.h"
#include <functional>
#include "emmy_debugger/emmy_facade.h"
#include "nlohmann/json.hpp"

Transporter::Transporter(bool server):
	receiveSize(0),
	readHead(true),
	running(false),
	connected(false),
	serverMode(server)
{
	loop = uv_loop_new();
	bufSize = 10 * 1024;
	buf = static_cast<char*>(malloc(bufSize));
}

Transporter::~Transporter()
{
	if (buf)
	{
		free(buf);
	}
	Stop();
	if (thread.joinable())
		thread.join();
}

void Transporter::Send(int cmd, const nlohmann::json document)
{
	std::string documentText = document.dump(-1, ' ', false, nlohmann::detail::error_handler_t::ignore);
	Send(cmd, documentText.data(), documentText.size());
}

void Transporter::OnAfterRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
	if (nread < 0)
	{
		/* Error or EOF */
		free(buf->base);
		uv_close(reinterpret_cast<uv_handle_t*>(handle), nullptr);

		// on disconnect
		OnDisconnect();
		return;
	}

	if (nread == 0)
	{
		/* Everything OK, but nothing read. */
		free(buf->base);
		return;
	}

	Receive(buf->base, nread);
}

void Transporter::Receive(const char* data, size_t len)
{
	if (bufSize < len + receiveSize)
	{
		// maybe a bug
		buf = static_cast<char*>(realloc(buf, bufSize + len));
	}
	memcpy(buf + receiveSize, data, len);
	receiveSize += len;

	size_t pos = 0;
	while (true)
	{
		size_t start = pos;
		for (size_t i = pos; i < receiveSize; i++)
		{
			if (data[i] == '\n')
			{
				pos = i + 1;
				break;
			}
		}
		if (start != pos)
		{
			if (readHead)
			{
				// skip
			}
			else
			{
				std::string text(buf + start, pos - start);
				auto document = nlohmann::json::parse(text);
				// bug 如果lua代码执行结束,这里行为未定义
				OnReceiveMessage(document);
			}
			readHead = !readHead;
		}
		else break;
	}

	if (pos > 0)
	{
		memcpy(buf, buf + pos, receiveSize - pos);
		receiveSize -= pos;
	}
}

void Transporter::OnReceiveMessage(const nlohmann::json document)
{
	EmmyFacade::Get().OnReceiveMessage(document);
}

void Transporter::OnDisconnect()
{
	connected = false;
	readHead = true;
	receiveSize = 0;
	EmmyFacade::Get().OnDisconnect();
}

void Transporter::OnConnect(bool suc)
{
	connected = suc;
	readHead = true;
	receiveSize = 0;

	EmmyFacade::Get().OnConnect(suc);
}

bool Transporter::IsConnected() const
{
	return connected;
}

bool Transporter::IsServerMode() const
{
	return serverMode;
}

////////////////////////////////////////////////////////////////////////////////
// send data

typedef struct
{
	uv_write_t req;
	uv_buf_t buf;
	uv_stream_t* handler;
} write_req_t;

static void after_write(uv_write_t* req, int status)
{
	const auto* writeReq = reinterpret_cast<write_req_t*>(req);
	free(writeReq->buf.base);
	delete writeReq;
}

static void after_async(uv_handle_t* h)
{
	delete h;
}

static void async_write(uv_async_t* h)
{
	auto* writeReq = (write_req_t*)h->data;
	uv_write(&writeReq->req, writeReq->handler, &writeReq->buf, 1, after_write);
	uv_close((uv_handle_t*)h, after_async);
}

void Transporter::Send(uv_stream_t* handler, int cmd, const char* data, size_t len)
{
	if (!IsConnected())
	{
		return;
	}
	auto* writeReq = new write_req_t();
	char cmdValue[100];
	const int l1 = sprintf(cmdValue, "%d\n", cmd);
	const size_t newLen = len + l1 + 1;
	char* newData = static_cast<char*>(malloc(newLen));
	// line1
	memcpy(newData, cmdValue, l1);
	// line2
	memcpy(newData + l1, data, len);
	newData[newLen - 1] = '\n';
	writeReq->buf = uv_buf_init(newData, newLen);
	writeReq->handler = handler;

	// thread safe:
	auto* async = new uv_async_t;
	async->data = writeReq;
	uv_async_init(loop, async, async_write);
	uv_async_send(async);
}

void Transporter::Send(uv_stream_t* handler, const char* data, size_t len)
{
	if (!IsConnected())
	{
		return;
	}
	auto* writeReq = new write_req_t();
	char* newData = static_cast<char*>(malloc(len));

	memcpy(newData, data, len);
	writeReq->buf = uv_buf_init(newData, len);
	writeReq->handler = handler;

	// thread safe:
	auto* async = new uv_async_t;
	async->data = writeReq;
	uv_async_init(loop, async, async_write);
	uv_async_send(async);
}

void Transporter::StartEventLoop()
{
	thread = std::thread(std::bind(&Transporter::Run, this));
}

void Transporter::Run()
{
	running = true;
	while (running)
	{
		uv_run(loop, UV_RUN_NOWAIT);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

int Transporter::Stop()
{
	running = false;
	return 0;
}

bool Transporter::ParseSocketAddress(const std::string &host, int port, sockaddr_storage *addr, std::string &err) 
{
	auto const loop = uv_default_loop();
	uv_getaddrinfo_t resolver;
	int res = uv_getaddrinfo(loop, &resolver, nullptr, host.c_str(),
	                         std::to_string(port).c_str(), nullptr);
	if (res != 0) {
		err = "Invalid host: ";
		err += uv_strerror(res);
		return false;
	}
	memcpy(addr, resolver.addrinfo->ai_addr, resolver.addrinfo->ai_addrlen);
	uv_freeaddrinfo(resolver.addrinfo);
	return true;
}

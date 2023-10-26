#include "emmy_debugger/proto/proto_handler.h"

#include "emmy_debugger/emmy_facade.h"
#include "emmy_debugger/transporter/transporter.h"
#include "nlohmann/json.hpp"

ProtoHandler::ProtoHandler(EmmyFacade *owner)
	: _owner(owner) {
}

void ProtoHandler::OnDispatch(nlohmann::json document) {
	if (document["cmd"].is_number_integer()) {
		switch (document["cmd"].get<MessageCMD>()) {
			case MessageCMD::InitReq: {
				InitParams params;
				params.Deserialize(document);
				OnInitReq(params);
				break;
			}
			case MessageCMD::ReadyReq: {
				OnReadyReq();
				break;
			}
			case MessageCMD::AddBreakPointReq: {
				AddBreakpointParams params;
				params.Deserialize(document);
				OnAddBreakPointReq(params);
				break;
			}
			case MessageCMD::RemoveBreakPointReq: {
				RemoveBreakpointParams params;
				params.Deserialize(document);
				OnRemoveBreakPointReq(params);
				break;
			}
			case MessageCMD::ActionReq: {
				ActionParams params;
				params.Deserialize(document);
				OnActionReq(params);
				break;
			}
			case MessageCMD::EvalReq: {
				EvalParams params;
				params.Deserialize(document);
				OnEvalReq(params);
				break;
			}
			default:
				break;
		}
	}
}

void ProtoHandler::OnInitReq(InitParams &params) {
	_owner->InitReq(params);
}

void ProtoHandler::OnReadyReq() {
	_owner->ReadyReq();
}

void ProtoHandler::OnAddBreakPointReq(AddBreakpointParams &params) {
	auto &manager = _owner->GetDebugManager();
	if (params.clear) {
		manager.RemoveAllBreakpoints();
	}

	for (auto &bp: params.breakPoints) {
		manager.AddBreakpoint(bp);
	}
}

void ProtoHandler::OnRemoveBreakPointReq(RemoveBreakpointParams &params) {
	auto &manager = _owner->GetDebugManager();
	for (auto bp: params.breakPoints) {
		manager.RemoveBreakpoint(bp->file, bp->line);
	}
}

void ProtoHandler::OnActionReq(ActionParams &params) {
	auto &manager = _owner->GetDebugManager();
	manager.DoAction(params.action);
}

void ProtoHandler::OnEvalReq(EvalParams &params) {
	auto &manager = _owner->GetDebugManager();
	manager.Eval(params.ctx);
}

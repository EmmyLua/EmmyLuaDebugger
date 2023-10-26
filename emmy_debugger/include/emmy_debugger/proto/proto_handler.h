#pragma once
#include "proto.h"
#include "nlohmann/json_fwd.hpp"

class EmmyFacade;

class ProtoHandler {
public:
	explicit ProtoHandler(EmmyFacade *owner);

	void OnDispatch(nlohmann::json params);

private:
	void OnInitReq(InitParams &params);

	void OnReadyReq();

	void OnAddBreakPointReq(AddBreakpointParams &params);

	void OnRemoveBreakPointReq(RemoveBreakpointParams &params);

	void OnActionReq(ActionParams& params);

	void OnEvalReq(EvalParams& params);

	EmmyFacade *_owner;
};

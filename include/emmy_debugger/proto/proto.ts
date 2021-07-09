// describe debugger proto

enum ValueType {
    VBool,
    VString,
    VTable,
    VFunction,
    VThread,
    VUserdata
}

enum VariableNameType {
    NString, NNumber, NComplex, 
}

interface Variable {
    type: ValueType;
    name: string;
    nameType: VariableNameType;
    value: string;
    children?: Variable[];
}

interface Stack {
    level: number;
    file: string;
    functionName: string;
    line: number;
    localVariables: Variable[];
    upvalueVariables: Variable[];
}

interface BreakPoint {
    file: string;
    line: number;
    condtion: string;
    hitCount: number;
}

interface InitRsp {
    version: string;
}

// add breakpoint
interface AddBreakPointReq {
    breakPoints: BreakPoint[];
}
interface AddBreakPointRsp {
}

// remove breakpoint
interface RemoveBreakPointReq {
    breakPoints: BreakPoint[];
}
interface RemoveBreakPointRsp {
}

enum DebugAction {
    Break,
    Continue,
    StepOver,
    StepIn,
    StepOut,
    Stop,
}

// break, continue, step over, step into, step out, stop
interface ActionReq {
    action: DebugAction;
}
interface ActionRsp {
}

// on break
interface BreakNotify {
    stacks: Stack[];
}

interface EvalReq {
    seq: number;
    expr: string;
    stackLevel: number;
}

interface EvalRsp {
    seq: number;
    success: boolean;
    error: string;
    value: Variable;
}
#include <iostream>
#include <map>
#include <cstring>
#include <vector>
#include <limits>

#include "antlr4-runtime.h"
#include "ELLexer.h"
#include "ELParser.h"
#include "ELBaseListener.h"

#include "Bytecodes.hpp"

#include "BytecodeCompiler.hpp"

using namespace std;
using namespace antlr4;

#if defined(EL_IS_BIG_ENDIAN)
#define convertToBigEndian(x)
#else
#if defined(__linux__)
#include <byteswap.h>
#define convertToBigEndian(x) x = bswap_64(x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define convertToBigEndian(x) x = OSSwapInt64(x)
#elif defined(_MSC_VER)
#include <stdlib.h>
#define convertToBigEndian(x) x = _byteswap_uint64(x)
#else
#error Platform does not provide convertToBigEndian functionality
#endif
#endif

// Instruction
int64_t Instruction::encodingLength() {
    return SIZEOF_BYTE + instructionDataLength();
}
string Instruction::getName() {
    return _name;
}
string Instruction::getDisplayString() {
    return getName();
}
void Instruction::encode(class MyListener *listener, class Function *function, ofstream *stream) {
    stream->write((char *)&_bytecode, 1);
    encodeData(listener, function, stream);
}
int64_t Instruction::instructionDataLength() {
    return 0;
}
bool Instruction::isLabel() {
    return false;
}
void Instruction::write(ofstream *stream, int64_t value) {
    convertToBigEndian(value);
    stream->write((char *)&value, sizeof(int64_t));
}
void Instruction::write(ofstream *stream, string value) {
    stream->write(value.data(), value.length());
}

// OneArgInstruction
string OneArgInstruction::getDisplayString() {
    return getName() + " " + to_string(_arg);
}
int64_t OneArgInstruction::instructionDataLength() {
    return SIZEOF_LONG;
}
void OneArgInstruction::encodeData(class MyListener *listener, class Function *function, ofstream *stream) {
    write(stream, _arg);
}

// CallInstruction
string CallInstruction::getDisplayString() {
    return getName() + " " + _functionName + " " + to_string(_argCount);
}
int64_t CallInstruction::instructionDataLength() {
    return SIZEOF_LONG + SIZEOF_LONG;
}
void CallInstruction::encodeData(MyListener *listener, Function *function, ofstream *stream) {
    map<string, Function> functions = listener->getFunctions();
    map<string,Function>::iterator it;
    it = functions.find(_functionName);
    if (it == functions.end()) {
        //error
    }
    int64_t functionID = it->second.getFunctionID();
    write(stream, functionID);
    write(stream, _argCount);
}

// JumpInstruction
int64_t JumpInstruction::instructionDataLength() {
    return SIZEOF_LONG;
}
string JumpInstruction::getDisplayString() {
    return getName() + " " + _labelName;
}
string JumpInstruction::getLabelName() {
    return _labelName;
}
void JumpInstruction::encodeData(class MyListener *listener, class Function *function, ofstream *stream) {
    write(stream, function->getLabelAddress(_labelName));
}

// PrintStringInstruction
int64_t PrintStringInstruction::instructionDataLength() {
    return SIZEOF_LONG;
}
string PrintStringInstruction::getDisplayString() {
    return getName() + " " + _text;
}
void PrintStringInstruction::encodeData(MyListener *listener, Function *function, ofstream *stream) {
    int64_t stringID = listener->getStringID(_encodedText);
    write(stream, stringID);
}

// Function
int64_t Function::getFunctionID() {
    return _functionID;
}
void Function::addInstruction(Instruction *ins) {
    _functionSize += ins->encodingLength();
    _instructions.push_back(ins);
}
void Function::addLabel(string labelName) {
    _labels.insert(pair<string,int64_t>(labelName,_functionSize));
}
int64_t Function::getFunctionSize() {
    return _functionSize;
}
vector<Instruction *> Function::getInstructions() {
    return _instructions;
}

int64_t Function::getLabelAddress(string label) {
    map<string,int64_t>::iterator it;
    it = _labels.find(label);
    if (it == _labels.end()) {
        //error
    }
    return it->second;
}

// MyListener
void MyListener::enterProgram(ELParser::ProgramContext *ctx) {
   _programName = ctx->programName()->NAME()->getText();
}

void MyListener::enterFunctionDeclaration(ELParser::FunctionDeclarationContext *ctx) {
    string functionName = ctx->functionName()->getText();

    map<string,Function>::iterator it;
    it = _functions.find(functionName);
    if (it != _functions.end()) {
        cerr << "Error: duplicate function named " << functionName << endl;
        exit(-9);
    }

    int64_t argCount = (int64_t)atol(ctx->functionArgCount()->integer()->getText().c_str());
    int64_t functionID = _functionCount++;
    Function func(functionName, functionID, argCount);
    ELParser::FunctionBodyContext *body = ctx->functionBody();
    vector<ELParser::InstructionContext *> instructions = body->instruction();
    for (vector<ELParser::InstructionContext *>::iterator it = instructions.begin(); it != instructions.end(); ++it) {
        ELParser::InstructionContext *insCtx = *it;
        Instruction *ins = nullptr;

        ELParser::NoArgInstructionContext *noArg = insCtx->noArgInstruction();
        ELParser::OneArgInstructionContext *oneArg = insCtx->oneArgInstruction();
        ELParser::CallInstructionContext *call = insCtx->callInstruction();
        ELParser::JumpInstructionContext *jump = insCtx->jumpInstruction();
        ELParser::PrintStringInstructionContext *printString = insCtx->printStringInstruction();
        ELParser::LabelContext *label = insCtx->label();

        if (noArg != nullptr) {
            ins = new (nothrow) Instruction(Bytecode::getBytecode(noArg->instructionName()->getText()));
        } else if (oneArg != nullptr) {
            ins = new (nothrow) OneArgInstruction(Bytecode::getBytecode(oneArg->instructionName()->getText()), (int64_t)atol(oneArg->integer()->getText().c_str()));
        } else if (call != nullptr) {
            string functionName = call->functionName()->getText();
            string argCount = call->integer()->getText();
            ins = new (nothrow) CallInstruction(Bytecode::getBytecode(call->instructionName()->getText()), functionName, (int64_t)atol(argCount.c_str()));
        } else if (jump != nullptr) {
            ins = new (nothrow) JumpInstruction(Bytecode::getBytecode(jump->instructionName()->getText()), jump->labelName()->getText());
        } else if (label != nullptr) {
            func.addLabel(label->labelName()->getText());
            continue;
        } else if (printString != nullptr) {
            ins = new (nothrow) PrintStringInstruction(Bytecode::getBytecode(printString->instructionName()->getText()), printString->string()->getText());
        } else {
            fprintf(stderr, "Unexpected instruction parsed\n");
            exit(-9);
        }
        if (nullptr == ins) {
            cerr << "Error: could not allocate Instruction\n";
            exit(-10);
        }
        func.addInstruction(ins);
    }

    _functions.insert(pair<string, Function>(functionName, func));
}

const char *MyListener::getProgramName() {
    return _programName.c_str();
}

map<string, Function> MyListener::getFunctions() {
    return _functions;
}

int64_t MyListener::getStringID(string str) {
    map<string,int64_t>::iterator it;
    it = _strings.find(str);
    if (it == _strings.end()) {
        int64_t stringID = _stringCount;
        _strings.insert(pair<string,int64_t>(str,_stringCount++));
        return stringID;
    } else {
        return it->second;
    }
}

string getFileName(const string s) {
   char sep = '/';
#ifdef _WIN32
   sep = '\\';
#endif

   size_t i = s.rfind(sep, s.length());
   if (i != string::npos) {
      return s.substr(i+1, s.length() - i);
   }

   return s;
}

string getOutputName(string inputName) {
    string fileName = getFileName(inputName);
    size_t extensionPos = fileName.find(".el");
    if (extensionPos == string::npos) {
        return "";
    }
    return fileName.replace(extensionPos, 3, ".le");
}

#define EYECATCHER "ELLE"

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        cerr << "No file specified" << endl;
        return -1;
    }
    string inputName = argv[1];
    string outputName = getOutputName(inputName);
    if (outputName.length() == 0) {
        cerr << "Improper EL filename: " << inputName << endl;
        return -2;
    }

    ifstream stream;
    stream.open(inputName);
    if (!stream.good()) {
        cerr << "Error opening " << inputName << endl;
        return -3;
    }

    ofstream compiledFile;
    compiledFile.open(outputName, ios::out | ios::binary);
    if (!compiledFile.good()) {
        cerr << "Error opening output file" << outputName << endl;
        return -4;
    }

    cout << "Compiling " << inputName << " to " << outputName << endl;

    ANTLRInputStream input(stream);
    ELLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    ELParser parser(&tokens);

    ELParser::ProgramContext* tree = parser.program();
    MyListener listener;
    tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

    compiledFile.write(EYECATCHER, strlen(EYECATCHER));

    const char *programName = listener.getProgramName();
    size_t programNameLength = strlen(programName);
    if (programNameLength > 127) {
        cerr << "Error program name is limited to 127 characters" << endl;
        return -5;
    }
    compiledFile.write((char *)&programNameLength, 1);
    compiledFile.write(programName, programNameLength);

    int64_t functionCount = listener.getFunctionCount();
    if (functionCount > 127) {
        cerr << "Error number of functions is limited to 127" << endl;
        return -6;
    }
    compiledFile.write((char *)&functionCount, 1);

    map<string, Function> functions = listener.getFunctions();
    map<string,Function>::iterator it;
    for (it = functions.begin(); it != functions.end(); ++it) {
        Function *func = &it->second;
        const char *functionName = it->first.c_str();
        size_t functionNameLength = strlen(functionName);
        if (functionNameLength > 127) {
             cerr << "Error function name is limited to 127 characters" << endl;
             return -7;
         }
        compiledFile.write((char *)&functionNameLength, 1);
        compiledFile.write(functionName, functionNameLength);

        int64_t functionID = it->second.getFunctionID();
        convertToBigEndian(functionID);
        compiledFile.write((char *)&functionID, sizeof(int64_t));

        int64_t argCount = it->second.getArgCount();
        convertToBigEndian(argCount);
        compiledFile.write((char *)&argCount, sizeof(int64_t));

        int64_t functionSize = it->second.getFunctionSize();
        convertToBigEndian(functionSize);
        compiledFile.write((char *)&functionSize, sizeof(int64_t));

        vector<Instruction *> instructions = it->second.getInstructions();
        for(vector<Instruction *>::iterator it = instructions.begin(); it != instructions.end(); ++it) {
            Instruction *instruction = *it;
            instruction->encode(&listener, func, &compiledFile);
        }
    }

    //Write strings out
    int64_t stringCount = listener.getStringCount();
    convertToBigEndian(stringCount);
    compiledFile.write((char *)&stringCount, sizeof(int64_t));

    map<string, int64_t> strings = listener.getStrings();
    map<string,int64_t>::iterator iter;
    for (iter = strings.begin(); iter != strings.end(); ++iter) {
        int64_t stringID = iter->second;
        convertToBigEndian(stringID);
        compiledFile.write((char *)&stringID, sizeof(int64_t));
        int64_t stringLength = iter->first.length();
        convertToBigEndian(stringLength);
        compiledFile.write((char *)&stringLength, sizeof(int64_t));
        compiledFile.write(iter->first.data(), iter->first.length());
    }

    //Write out the eyecatcher again
    compiledFile.write(EYECATCHER, strlen("ELLE"));
    compiledFile.close();
    stream.close();

    return 0;
}

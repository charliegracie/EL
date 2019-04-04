#include <string>
#include <map>
#include <vector>

using namespace std;
using namespace antlr4;

class Instruction {
public:
    Instruction(Bytecodes bytecode) :
        _bytecode(bytecode),
        _name(Bytecode::getBytecodeName(bytecode))
    {}
    virtual ~Instruction() {};
    int64_t encodingLength();
    string getName();
    virtual string getDisplayString();
    void encode(class MyListener *listener, class Function *function, ofstream *stream);
protected:
    static const int SIZEOF_BYTE = 1;
    static const int SIZEOF_LONG = 8;

    Bytecodes getBytecode() { return _bytecode; }
    virtual int64_t instructionDataLength();
    virtual bool isLabel();
    virtual void encodeData(class MyListener *listener, class Function *function, ofstream *stream){}
    void write(ofstream *stream, int64_t value);
    void write(ofstream *stream, string value);
private:
    Bytecodes _bytecode;
    string _name;
};

class OneArgInstruction : public Instruction {
public:
    OneArgInstruction(Bytecodes bytecode, int64_t arg) :
        Instruction(bytecode),
        _arg(arg)
    {}
    virtual string getDisplayString();
protected:
    virtual int64_t instructionDataLength();
    virtual void encodeData(class MyListener *listener, class Function *function, ofstream *stream);
private:
    int64_t _arg;
};

class CallInstruction : public Instruction {
public:
    CallInstruction(Bytecodes bytecode, string functionName, int64_t argCount) :
        Instruction(bytecode),
        _functionName(functionName),
        _argCount(argCount)
    {}
    virtual string getDisplayString();
protected:
    virtual int64_t instructionDataLength();
    virtual void encodeData(class MyListener *listener, class Function *function, ofstream *stream);
private:
    string _functionName;
    int64_t _argCount;
};

class JumpInstruction : public Instruction {
public:
    JumpInstruction(Bytecodes bytecode, string labelName) :
        Instruction(bytecode),
        _labelName(labelName)
    {}
    virtual int64_t instructionDataLength();
    virtual string getDisplayString();
    string getLabelName();
protected:
    virtual void encodeData(class MyListener *listener, class Function *function, ofstream *stream);
private:
    string _labelName;
};

class PrintStringInstruction : public Instruction {
public:
    PrintStringInstruction(Bytecodes bytecode, string text) :
        Instruction(bytecode),
        _text(text),
        _encodedText(text.substr(1, text.length()-2))
    {
        string toReplace("\\n");
        size_t pos = _encodedText.find(toReplace);
        while (pos != std::string::npos) {
            _encodedText = _encodedText.replace(pos, toReplace.length(), 1, '\n');
            pos = _encodedText.find(toReplace);
        }
    }
    virtual int64_t instructionDataLength();
    virtual string getDisplayString();
    string getEncodedText() { return _encodedText; }
protected:
    void encodeData(MyListener *listener, Function *function, ofstream *stream);
private:
    string _text;
    string _encodedText;
};

class Function {
public:
    Function(string functionName, int64_t functionID, int64_t argCount) :
        _functionName(functionName),
        _functionID(functionID),
        _argCount(argCount),
        _functionSize(0),
        _instructions(),
        _labels()
    {};
    void addInstruction(Instruction *ins);
    void addLabel(string labelName);
    int64_t getFunctionID();
    int64_t getArgCount() { return _argCount; }
    int64_t getFunctionSize();
    vector<Instruction *> getInstructions();
    int64_t getLabelAddress(string label);

private:
    string _functionName;
    int64_t _functionID;
    int64_t _argCount;
    int64_t _functionSize;
    vector<Instruction *> _instructions;
    map<string, int64_t> _labels;
};

class MyListener : public ELBaseListener {
public:
    MyListener() :
        _programName(),
        _functionCount(0),
        _stringCount(0),
        _functions(),
        _strings()
    {}
    void enterProgram(ELParser::ProgramContext *ctx) override;
    void enterFunctionDeclaration(ELParser::FunctionDeclarationContext *ctx) override;
    const char *getProgramName();
    map<string, Function> getFunctions();
    int64_t getFunctionCount() { return _functionCount; }
    int64_t getStringID(string str);
    int64_t getStringCount() { return _stringCount; }
    map<string,int64_t> getStrings() { return _strings; }

private:
    string _programName;
    int64_t _functionCount;
    int64_t _stringCount;
    map<string, Function> _functions;
    map<string, int64_t> _strings;
};

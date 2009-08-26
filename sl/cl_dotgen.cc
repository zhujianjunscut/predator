#include "cl_dotgen.hh"
#include "cl_private.hh"

#include <libgen.h>

#include <fstream>
#include <map>
#include <set>

class ClDotGenerator: public ICodeListener {
    public:
        ClDotGenerator();
        ClDotGenerator(const char *glDotFile);
        virtual ~ClDotGenerator();

        virtual void file_open(
            const char              *file_name);

        virtual void file_close();

        virtual void fnc_open(
            const struct cl_location*loc,
            const char              *fnc_name,
            enum cl_scope_e         scope);

        virtual void fnc_arg_decl(
            int                     arg_id,
            const char              *arg_name);

        virtual void fnc_close();

        virtual void bb_open(
            const char              *bb_name);

        virtual void insn(
            const struct cl_insn    *cli);

        virtual void insn_call_open(
            const struct cl_location*loc,
            const struct cl_operand *dst,
            const struct cl_operand *fnc);

        virtual void insn_call_arg(
            int                     arg_id,
            const struct cl_operand *arg_src);

        virtual void insn_call_close();

        virtual void insn_switch_open(
            const struct cl_location*loc,
            const struct cl_operand *src);

        virtual void insn_switch_case(
            const struct cl_location*loc,
            const struct cl_operand *val_lo,
            const struct cl_operand *val_hi,
            const char              *label);

        virtual void insn_switch_close();

    private:
        bool                    hasGlDotFile_;
        std::ofstream           glOut_;
        std::ofstream           perFileOut_;
        std::ofstream           perFncOut_;

        Location                loc_;
        std::string             fnc_;
        std::string             bb_;

    private:
        enum NodeType {
            NT_PLAIN,
            NT_ENTRY,
            NT_RET,
            NT_ABORT
        };

        enum EdgeType {
            ET_JMP,
            ET_COND_THEN,
            ET_COND_ELSE,
            ET_SWITCH_CASE,
            ET_LC_CALL,
            ET_GL_CALL
        };

        typedef std::set<std::string>                   TCallSet;
        typedef std::map<std::string, TCallSet>         TCallMap;
        typedef std::map<std::string, bool>             TExternMap;
        typedef std::map<std::string, EdgeType>         TEdgeMap;
        TEdgeMap                edgeMap_;
        NodeType                nodeType_;

    private:
        void createDotFile(std::ofstream &str, std::string fileName);
        void gobbleEdge(std::string dst, EdgeType type);
        void emitEdge(std::string dst, EdgeType type);
        void emitBb();
};

// TODO: chain streams using boost::iostreams instead of the ugly macro
#define FILE_FNC_STREAM(what) do { \
    perFileOut_ << what; \
    perFncOut_ << what; \
} while (0)

#define SL_QUOTE(what) \
    "\"" << what << "\""

// TODO: get rid of
#define SL_QUOTE_BB(bb) \
    SL_QUOTE(fnc_ << "." << bb)

#define SL_DOT_SUFFIX ".svg"

#define SL_QUOTE_URL(fnc) \
    SL_QUOTE(basename((char *) loc_.currentFile.c_str()) \
    << "-" << fnc << SL_DOT_SUFFIX)

#define SL_GRAPH(name) \
    "digraph " << SL_QUOTE(name) << " {" << std::endl \
    << "\tlabel=<<FONT POINT-SIZE=\"36\">" << name << "</FONT>>;" << std::endl \
    << "\tlabelloc=t;" << std::endl

#define SL_SUBGRAPH(name, label) \
    "subgraph \"cluster" << name << "\" {" << std::endl \
    << "\tlabel=" << SL_QUOTE(label) << ";" << std::endl

using std::ios;
using std::string;

// /////////////////////////////////////////////////////////////////////////////
// ClDotGenerator implementation
ClDotGenerator::ClDotGenerator():
    hasGlDotFile_(false),
    nodeType_(NT_PLAIN)
{
}

void ClDotGenerator::createDotFile(std::ofstream &str, std::string fileName) {
    fileName += ".dot";
    str.open(fileName.c_str(), ios::out);
    if (str)
        return;

    CL_MSG_STREAM_INTERNAL(cl_error, "error: "
                           "unable to create file '" << fileName << "'");
}

ClDotGenerator::ClDotGenerator(const char *glDotFile):
    hasGlDotFile_(true),
    nodeType_(NT_PLAIN)
{
    glOut_.open(glDotFile, ios::out);
    if (!glOut_) {
        CL_MSG_STREAM_INTERNAL(cl_error, "error: "
                               "unable to create file '" << glDotFile << "'");
        return;
    }

    glOut_ << SL_GRAPH(glDotFile);
}

ClDotGenerator::~ClDotGenerator() {
    glOut_ << "}" << std::endl;
}

void ClDotGenerator::gobbleEdge(std::string dst, EdgeType edgeType) {
    switch (edgeType) {
        case ET_JMP:
            if (bb_.empty())
                // skip jmp to ENTRY
                break;

            // fall through!!
        default:
            edgeMap_[dst] = edgeType;
    }
}

void ClDotGenerator::emitEdge(std::string dst, EdgeType edgeType) {
    if (ET_LC_CALL == edgeType) {
        FILE_FNC_STREAM("\t" << SL_QUOTE_BB(dst)
                << " [label=" << SL_QUOTE(dst)
                << ", color=blue, URL=" << SL_QUOTE_URL(dst) << "];"
                << std::endl);

        FILE_FNC_STREAM("\t" << SL_QUOTE_BB(bb_) << " -> " << SL_QUOTE_BB(dst)
                << " [color=blue];"
                << std::endl);

        glOut_ << "\t" << SL_QUOTE(fnc_)
                << " -> " << SL_QUOTE(dst)
                << " [color=blue];" << std::endl;

        return;
    }

    FILE_FNC_STREAM("\t" << SL_QUOTE_BB(bb_) << " -> " << SL_QUOTE_BB(dst)
            << " [color=");
    switch (edgeType) {
        case ET_JMP:            FILE_FNC_STREAM("black");   break;
        case ET_COND_THEN:
        case ET_COND_ELSE:      FILE_FNC_STREAM("green");   break;
        case ET_SWITCH_CASE:    FILE_FNC_STREAM("yellow");  break;
        case ET_GL_CALL:        FILE_FNC_STREAM("red");     break;

        // just to make compiler happy
        case ET_LC_CALL:
            CL_INTERNAL_ERROR("unexpected edge type");
            break;
    }
    FILE_FNC_STREAM("];" << std::endl);
}

void ClDotGenerator::emitBb() {
    // colorize current BB node
    FILE_FNC_STREAM("\t" << SL_QUOTE_BB(bb_) << " [color=");
    switch (nodeType_) {
        case NT_PLAIN:          FILE_FNC_STREAM("black");   break;
        case NT_ENTRY:
        case NT_RET:            FILE_FNC_STREAM("blue");    break;
        case NT_ABORT:          FILE_FNC_STREAM("red");     break;
    }
    FILE_FNC_STREAM(", label=" << SL_QUOTE(bb_) << "];" << std::endl);

    // emit all BB edges
    TEdgeMap::iterator i;
    for (i = edgeMap_.begin(); i != edgeMap_.end(); ++i)
        this->emitEdge(i->first, i->second);

    // clear emitted edges
    edgeMap_.clear();
}

void ClDotGenerator::file_open(
            const char              *file_name)
{
    loc_.currentFile = file_name;
    this->createDotFile(perFileOut_, file_name);
    perFileOut_ << SL_GRAPH(file_name);

    glOut_ << SL_SUBGRAPH(file_name, file_name) << "\tcolor=red;" << std::endl
        << "\tlabel " << SL_QUOTE(basename((char *) loc_.currentFile.c_str())
        << SL_DOT_SUFFIX) << std::endl;
}

void ClDotGenerator::file_close()
{
    glOut_ << "}" << std::endl;

    perFileOut_ << "}" << std::endl;
    if (!perFileOut_)
        CL_MSG_STREAM_INTERNAL(cl_warning, "warning: "
                "error detected while closing a file");

    perFileOut_.close();
}

void ClDotGenerator::fnc_open(
            const struct cl_location*loc,
            const char              *fnc_name,
            enum cl_scope_e         scope)
{
    loc_ = loc;
    fnc_ = fnc_name;
    this->createDotFile(perFncOut_, string(loc_.currentFile) + "-" + fnc_name);
    perFncOut_ << SL_GRAPH(fnc_name << "() at "
            << loc_.locFile << ":" << loc_.locLine);

    glOut_ << "\t" << SL_QUOTE(fnc_name)
            << " [label=" << SL_QUOTE(fnc_name)
            << ", color=blue, URL=" << SL_QUOTE_URL(fnc_name) << "];"
            << std::endl;
}

void ClDotGenerator::fnc_arg_decl(
            int                     arg_id,
            const char              *arg_name)
{
}

void ClDotGenerator::fnc_close()
{
    if (!bb_.empty())
        // emit previous bb
        this->emitBb();

    FILE_FNC_STREAM("}" << std::endl);
    if (!perFncOut_)
        CL_MSG_STREAM_INTERNAL(cl_warning, "warning: "
                "error detected while closing a file");

    perFncOut_.close();
    bb_.clear();
}

void ClDotGenerator::bb_open(
            const char              *bb_name)
{
    // FIXME: the condition is not going to work well in general
    if (bb_.empty()) {
        nodeType_ = NT_ENTRY;

        perFileOut_ << SL_SUBGRAPH(
                fnc_ << "." << bb_name,
                fnc_  << "() at " << loc_.locFile << ":" << loc_.locLine)
            << "\tcolor=blue;" << std::endl
            << "\tURL=" << SL_QUOTE_URL(fnc_) << ";" << std::endl;

    } else {
        // emit last BB
        this->emitBb();
        nodeType_ = NT_PLAIN;
    }

    bb_ = bb_name;
}

void ClDotGenerator::insn(
            const struct cl_insn    *cli)
{
    switch (cli->type) {
        case CL_INSN_JMP:
            this->gobbleEdge(cli->data.insn_jmp.label, ET_JMP);
            break;

        case CL_INSN_COND:
            this->gobbleEdge(cli->data.insn_cond.then_label, ET_COND_THEN);
            this->gobbleEdge(cli->data.insn_cond.else_label, ET_COND_ELSE);
            break;

        case CL_INSN_RET:
            nodeType_ = NT_RET;
            break;

        case CL_INSN_ABORT:
            nodeType_ = NT_ABORT;
            break;

        case CL_INSN_UNOP:
        case CL_INSN_BINOP:
            break;
    }
}

void ClDotGenerator::insn_call_open(
            const struct cl_location*loc,
            const struct cl_operand *dst,
            const struct cl_operand *fnc)
{
    if (fnc->type != CL_OPERAND_FNC || fnc->deref || fnc->offset) {
        CL_MSG_STREAM(cl_warn, LocationWriter(loc, &loc_) << "warning: "
                      "ClDotGenerator: unhandled call");
        return;
    }

    // TODO: handle decl location?
    this->gobbleEdge(fnc->data.fnc.name, (fnc->data.fnc.is_extern)
            ? ET_GL_CALL
            : ET_LC_CALL);
}

void ClDotGenerator::insn_call_arg(
            int                     arg_id,
            const struct cl_operand *arg_src)
{
}

void ClDotGenerator::insn_call_close()
{
}

void ClDotGenerator::insn_switch_open(
            const struct cl_location*loc,
            const struct cl_operand *src)
{
}

void ClDotGenerator::insn_switch_case(
            const struct cl_location*loc,
            const struct cl_operand *val_lo,
            const struct cl_operand *val_hi,
            const char              *label)
{
    this->gobbleEdge(label, ET_SWITCH_CASE);
}

void ClDotGenerator::insn_switch_close()
{
}

// /////////////////////////////////////////////////////////////////////////////
// public interface, see cl_dotgen.hh for more details
ICodeListener* createClDotGenerator(const char *args) {
    return (args && *args)
        ? new ClDotGenerator(args)
        : new ClDotGenerator();
}

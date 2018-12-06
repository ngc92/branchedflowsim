//
// Created by erik on 12/7/17.
//

#include <unordered_map>
#include "usage.hpp"


namespace
{
    std::string type_name(args::ArgValueType type) {
        switch(type) {
            case args::ArgValueType::UNSPECIFIED:
                return "Flag";
            case args::ArgValueType::BOOLEAN:
                return "Bool";
            case args::ArgValueType::INTEGER:
                return "Integer";
            case args::ArgValueType::NUMBER:
                return "Number";
            case args::ArgValueType::STRING:
                return "String";
        }
        THROW_EXCEPTION(std::invalid_argument, "Invalid ArgValueType given.");
    }

    std::string amount_name(args::ArgValueCount count) {
        switch(count) {
            case args::ArgValueCount::NONE:
                return "0";
            case args::ArgValueCount::SINGLE:
                return "1";
            case args::ArgValueCount::MULTI:
                return "-1";
            case args::ArgValueCount::UNSPECIFIED:
            {
                THROW_EXCEPTION(std::invalid_argument, "ArgValueCount::UNSPECIFIED given.");
            }
        }
        THROW_EXCEPTION(std::invalid_argument, "Invalid ArgValueCount given.");
    }
    /*
     * Below we define visitor classes that generate there respective usage/help/spec strings by inserting into an
     * output stream. This works because we can process the different ArgumentSpec objects sequentially.
     */

    // -----------------------------------------------------------------------------------------------------------------
    //                                               Usage visitor
    // -----------------------------------------------------------------------------------------------------------------

    class UsageVisitor {
    public:
        explicit UsageVisitor(std::ostream &stream) : mStream(stream) {
        }

        void print_arg(const args::ArgumentSpec &arg) {

            if (!arg.is_required()) mStream << '[';

            if (arg.is_positional()) {
                boost::to_upper_copy(std::ostream_iterator<char>(mStream), arg.name());
            } else {
                mStream << arg.name();
                if (arg.value_count() != args::ArgValueCount::NONE) {
                    auto type = arg.value_type();
                    mStream << ' ';
                    if (type == args::ArgValueType::UNSPECIFIED) {
                        boost::to_upper_copy(std::ostream_iterator<char>(mStream), arg.name());
                    } else {
                        boost::to_upper_copy(std::ostream_iterator<char>(mStream), type_name(type));
                    }
                }
            }

            if (arg.value_count() == args::ArgValueCount::MULTI) {
                mStream << " ...";
            }

            if (!arg.is_required()) mStream << ']';
        }

        void print_space() {
            if (!mFirst) {
                mStream << ' ';
            }
            mFirst = false;
        }

        void print_name(const std::string &name) {
            mFirst = false;
            mStream << name;
        }

        void operator()(const args::ArgumentSpec &arg) {
            print_space();
            print_arg(arg);
        }

    private:
        std::ostream &mStream;
        bool mFirst = true;
    };

    // -----------------------------------------------------------------------------------------------------------------
    //                                               Help Message
    // -----------------------------------------------------------------------------------------------------------------

    class HelpVisitor
    {
    public:
        explicit HelpVisitor(std::ostream& stream) : mStream(stream) {
        }

        void operator()(const args::ArgumentSpec& arg) {
            if(!arg.description().empty()) {
                mStream << arg.name() << " (" << type_name(arg.value_type()) << "): " << arg.description() << "\n";
            } else {
                mStream << arg.name() << " (" << type_name(arg.value_type()) << ")\n";
            }
        }
    private:
        std::ostream& mStream;
    };


    // -----------------------------------------------------------------------------------------------------------------
    //                                               Python Glue
    // -----------------------------------------------------------------------------------------------------------------

    std::ostream& esc(std::ostream& target, const std::string& orig) {
        for(char c : orig) {
            if(c == '\n') {
                target << "\\n";
            } else if(c == '"') {
                target << "\"";
            } else {
                target.put(c);
            }
        }
        return target;
    }

    class PythonVisitor
    {
    public:
        explicit PythonVisitor(std::ostream& stream) : mStream(stream) {
        }

        void operator()(const args::ArgumentSpec& arg) {
            if(!mFirst) mStream << ", ";
            mFirst = false;

            mStream << "ArgSpec('" << arg.name() << "', \"";
            esc(mStream, arg.description());
            mStream << "\", positional=" << arg.is_positional()
                    << ", required=" << arg.is_required()
                    << ", type='" << type_name(arg.value_type())
                    << "', amount=" << amount_name(arg.value_count())
                    << ")\n";
        }
    private:
        std::ostream& mStream;
        bool mFirst = true;
    };
}

// Now implement the public functions based on the helper classes above.

std::string args::usage_string(const args::ArgumentSet& args)
{
    std::stringstream str;
    usage_string(str, args);
    return str.str();
}

void args::usage_string(std::ostream& stream, const args::ArgumentSet& args)
{
    UsageVisitor visitor(stream);

    // add the name of the argument set if it is given.
    if(args.name() != "") {
        visitor.print_name(args.name());
    }
    args.visit_args(visitor);
}


void args::help_string(std::ostream& stream, const args::ArgumentSet& args)
{
    args.visit_args(HelpVisitor(stream));
}

std::string args::help_string(const args::ArgumentSet& args)
{
    std::stringstream str;
    help_string(str, args);
    return str.str();
}

void args::argspec_string(std::ostream& stream, const args::ArgumentSet& arguments)
{
    stream << '[';
    PythonVisitor v(stream);
    arguments.visit_args(v);
    stream << ']';
}

std::string args::argspec_string(const args::ArgumentSet& arguments) {
    std::stringstream str;
    argspec_string(str, arguments);
    return str.str();
}
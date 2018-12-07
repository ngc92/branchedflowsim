#include "factory.hpp"
#include "args/usage.hpp"

std::string indent(const std::string& str, std::size_t indent, std::size_t init)
{
    std::string cp = "";
    cp.reserve(str.size());
    
    std::size_t count = init;
    if(init < indent)
    {
        cp.append(indent - init, ' ');
        count = indent;
    }
    for(const char& c : str)
    {
        cp += c;
        count += 1;
        if(count == 78)
            cp += '\n';
        
        if(cp.back() == '\n')
        {
            cp.append(indent, ' ');
            count = indent;
        }
    }
    return cp;
}

std::string factory::detail::FactoryBase::getHelp(const std::string& type) const {
    auto arguments = get_arguments(type);
    return args::usage_string(arguments) + "\n" + arguments.description() + "\n" +
           args::help_string(arguments);
}

void factory::detail::FactoryBase::printHelp() const {
    for(const auto& type : getTypes())
    {
        std::cout << "  " << type << ": "<<
                  indent(getHelp(type), 4, type.size() + 4) << "\n\n";
    }

}

std::vector<std::string> factory::detail::FactoryBase::getTypes() const {
    // create new vector and reserve sufficient space.
    std::vector<std::string> types;
    types.reserve( mBuilders.size() );

    for(const auto& b : mBuilders) {
        types.emplace_back(b.first);
    }
    return move(types);
}

boost::any factory::detail::FactoryBase::get_builder_any(const std::string& type) const {
    auto creator = mBuilders.find(type);
    // check that the observer exists
    if( creator == mBuilders.end() )
    {
        THROW_EXCEPTION( std::runtime_error, "Unknown factory for type '%1%' requested!", type );
    }

    // if yes, call the c'tor function with the given args.
    return creator->second;
}

void factory::detail::FactoryBase::add_builder_any(std::string type, boost::any builder) {
    auto result = mBuilders.emplace(type, builder);
    if(!result.second) {
        THROW_EXCEPTION(std::logic_error, "A builder for type '%1%' is already registered.", type);
    }
}

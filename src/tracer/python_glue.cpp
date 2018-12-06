//
// Created by eriks on 12/8/17.
//
#include <initial_conditions/init_factory.hpp>
#include "observers/observer_factory.hpp"
#include "args/usage.hpp"
#include "factory/builder_base.hpp"
#include "factory/factory.hpp"

int main(int argc, const char* argv[]) {
    std::string kind;
    std::string command{};
    std::string type{};
    args::ArgumentSet arguments;
    arguments << args::ArgumentSpec("kind").positional().store(kind);
    arguments << args::ArgumentSpec("command").positional().store(command).optional();
    arguments << args::ArgumentSpec("type").positional().store(type).optional();

    arguments.parse(argv+1, argv+argc);

    if(kind == "observer") {
        if (command == "") {
            for (auto& type : getObserverFactory().getTypes()) {
                std::cout << type << "\n";
            }
            return EXIT_SUCCESS;
        } else if (command == "args") {
            args::argspec_string(std::cout, getObserverFactory().get_arguments(type));
            std::cout << std::endl;
            return EXIT_SUCCESS;
        } else if (command == "doc") {
            std::cout << getObserverFactory().get_arguments(type).description() << "\n\n";
            args::help_string(std::cout, getObserverFactory().get_arguments(type));
            return EXIT_SUCCESS;
        } else if (command == "monodromy") {
            std::cout << std::boolalpha << getObserverFactory().get_builder(type)->need_monodromy();
            return EXIT_SUCCESS;
        }
    } else if(kind == "incoming") {
        if (command == "") {
            for (auto& type : init_cond::getInitialConditionFactory().getTypes()) {
                std::cout << type << "\n";
            }
            return EXIT_SUCCESS;
        } else if (command == "args") {
            args::argspec_string(std::cout, init_cond::getInitialConditionFactory().get_arguments(type));
            std::cout << std::endl;
            return EXIT_SUCCESS;
        } else if (command == "doc") {
            std::cout << init_cond::getInitialConditionFactory().get_arguments(type).description() << "\n\n";
            args::help_string(std::cout, init_cond::getInitialConditionFactory().get_arguments(type));
            return EXIT_SUCCESS;
        }
    }

    return EXIT_FAILURE;
}
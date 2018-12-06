//
// Created by erik on 12/5/17.
//

#ifndef BRANCHEDFLOWSIM_ARGUMENT_SET_HPP
#define BRANCHEDFLOWSIM_ARGUMENT_SET_HPP

#include "spec.hpp"

namespace args
{
    /// Helper struct that allows comparisons between
    /// named objects (in particular args::Argument)
    /// so that they can be used in std::set.
    struct CompareByName
    {
        template<class T>
        bool operator()(const T& a, const T& b)
        {
            return a.name() < b.name();
        }
    };

    /*!
     * \class ArgumentSet
     * \brief A collection of parsable arguments.
     * \details A (named) set of arguments that can be used to parse
     *          command line configuration.
     *
     *          The arguments are sorted into required positional,
     *          optional positional and named arguments.
     */
    class ArgumentSet
    {
        using argset_t = std::set<ArgumentSpec, CompareByName>;
    public:
        /// constructor that sets the name of the argset.
        explicit ArgumentSet(std::string name = "");

        // build up
        /*! Add an argument to this argument set.
         *  \details Checks that `arg` is a well-formed argument and can
         *           and whether it can sensible added to the `ArgumentSet`.
         *  \throws std::logic_error if an argument named `arg` already exists.
         *  \throws std::logic_error if a FLAG argument is required or positional.
         *  \throws std::logic_error if a mandatory positional argument follows a positional argument that has
         *                           multiple values.
         */
        ArgumentSet& add_argument(ArgumentSpec arg);

        /*! Add an argument to this argument set.
         * \copydetails add_argument
         */
        ArgumentSet& operator<<(ArgumentSpec arg);

        const std::string& name() const { return mName; }
        const std::string& description() const { return mDescription; }

        /// Set the description of this argument set.
        ArgumentSet& description(std::string description);

        /*! \brief Apply `visitor` to all arguments.
         *  \details Visitor is first applied to required positional
         *           arguments, then to optional positional arguments
         *           and finally to the named arguments.
         */
        template<class Visitor>
        void visit_args(Visitor&& visitor) const
        {
            for (const auto& arg : mRequiredPositionalArguments) {
                visitor(arg);
            }
            for (const auto& arg : mOptionalPositionalArguments) {
                visitor(arg);
            }
            for (const auto& arg : mNamedArguments) {
                visitor(arg);
            }
        }

        /*!  parse arguments where tokens are supplied in an iterable
             container.
             \sa template<class Iterator> void parse(Iterator first, const Iterator& last) const
        */
        template<class Container>
        void parse(Container&& container) const {
            using std::begin;
            using std::end;
            parse(begin(container), end(container));
        }

        /*! \brief Parse arguments supplied as an iterator pair of tokens.
         *  \details Copies all tokens into a `std::vector<std::string>` and calls `parse_token_vector()` on that.
         *           This incurs some negligible overhead, but allows us to keep this generic interface but
         *           implement the parsing function specialised for string vectors, so we need not put it into
         *           a template function.
         *
         *           For directly parsing data already in an iteratable container the convenience method
         *           template<class Container> void parse(Container&& container) is provided.
         *  \tparam Iterator Forward iterator type. Its `value_type` should be convertible to string.
         */
        template<class Iterator>
        void parse(Iterator first, const Iterator& last) const
        {
            std::vector<std::string> cache;
            std::copy(first, last, std::back_inserter(cache));
            parse_token_vector(cache);
        }

    private:
        /// check whether `str` is name or alias of one of the named arguments.
        bool is_argument_name(const std::string& str) const;

        /// Iterator type for iterating the token vector.
        using TokenIterator = std::vector<std::string>::const_iterator;

        /// Parse arguments supplied as a vector of strings.
        void parse_token_vector(const std::vector<std::string> &tokens) const;

        // argument parser functions.
        /*! \brief parse a single argument.
         *  \details This parses a single argument `arg` and pushes the corresponding handlers into
         *           the `handlers` vector. The `first` iterator is advanced for each consumed token.
         *  \param arg The argument to parse.
         *  \param[in,out] first The iterator for argument values.
         *                       This will be advanced for each processed token.
         *  \param[in] last The end iterator for the tokens.
         *  \param[out] handlers The argument handlers are pushed here.
         */
        void parse_argument(const ArgumentSpec& arg, TokenIterator& first, const TokenIterator& last,
                            std::vector<HandlerFunction>& handlers) const;

        /// parse an agument that expects a single value token.
        bool parse_single_token_argument(const ArgumentValue& arg, TokenIterator& first,
                                         const TokenIterator& last, std::vector<HandlerFunction>& handlers) const;

        /// parse an agument that can process multiple value tokens.
        bool parse_multi_token_argument(const ArgumentValue& arg, TokenIterator& first,
                                        const TokenIterator& last, std::vector<HandlerFunction>& handlers) const;

        // saved arguments
        std::vector<ArgumentSpec> mRequiredPositionalArguments;
        std::vector<ArgumentSpec> mOptionalPositionalArguments;
        argset_t              mNamedArguments;

        // metadata
        std::string mName;
        std::string mDescription;
    };
}

#endif //BRANCHEDFLOWSIM_ARGUMENT_SET_HPP

#include <common.test.hpp>

#include <tavros/core/utils/preproc_lexer.hpp>

#include <map>
#include <set>
#include <cstdlib>

#include <vector>

using namespace tavros::core;

class preproc_lexer_test : public unittest_scope
{
};


using tt = preproc_token::token_type;

constexpr std::string_view to_str(tt type) noexcept
{
    using enum tt;

    switch (type)
    {
        case directive:         return "directive";
        case header_name:       return "header_name";
        case identifier:        return "identifier";
        case number:            return "number";
        case punctuation:       return "punctuation";
        case string_literal:    return "string_literal";
        case character_literal: return "character_literal";
        case error:             return "error";
        case end_of_file:       return "end_of_file";
    }

    // unreachable, но формально обязателен
    return "<unknown token_type>";
}

static std::vector<preproc_token> lex_all(string_view src)
{
    preproc_lexer lexer(src.data(), src.data() + src.size());

    std::vector<preproc_token> tokens;
    for (;;)
    {
        auto tok = lexer.next_token();
        tokens.push_back(tok);

        if (tok.type() == tt::end_of_file)
        {
            break;
        }
    }
    return tokens;
}

TEST_F(preproc_lexer_test, empty_source)
{
    auto tokens = lex_all("");

    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, identifiers_and_numbers)
{
    constexpr string_view src = "foo bar123 42";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4u);

    EXPECT_EQ(tokens[0].type(), tt::identifier);
    EXPECT_EQ(tokens[0].token(), "foo");

    EXPECT_EQ(tokens[1].type(), tt::identifier);
    EXPECT_EQ(tokens[1].token(), "bar123");

    EXPECT_EQ(tokens[2].type(), tt::number);
    EXPECT_EQ(tokens[2].token(), "42");

    EXPECT_EQ(tokens[3].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, preprocessor_directive)
{
    constexpr string_view src = R"(
        #include <some/path.h>   
        #    include   <  some/path2.h  >   
        #define FOO 123    
    )";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 8u);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");

    EXPECT_EQ(tokens[1].type(), tt::header_name);
    EXPECT_EQ(tokens[1].token(), "some/path.h");

    EXPECT_EQ(tokens[2].type(), tt::directive);
    EXPECT_EQ(tokens[2].token(), "include");

    EXPECT_EQ(tokens[3].type(), tt::header_name);
    EXPECT_EQ(tokens[3].token(), "  some/path2.h  ");

    EXPECT_EQ(tokens[4].type(), tt::directive);
    EXPECT_EQ(tokens[4].token(), "define");

    EXPECT_EQ(tokens[5].type(), tt::identifier);
    EXPECT_EQ(tokens[5].token(), "FOO");

    EXPECT_EQ(tokens[6].type(), tt::number);
    EXPECT_EQ(tokens[6].token(), "123");

    EXPECT_EQ(tokens[7].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, quoted_header_name)
{
    constexpr string_view src = "#   include      \"my_header.hpp\"  \n#include \"my_header2.hpp\"";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 5u);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");
    
    EXPECT_EQ(tokens[1].type(), tt::header_name);
    EXPECT_EQ(tokens[1].token(), "my_header.hpp");
    
    EXPECT_EQ(tokens[2].type(), tt::directive);
    EXPECT_EQ(tokens[2].token(), "include");

    EXPECT_EQ(tokens[3].type(), tt::header_name);
    EXPECT_EQ(tokens[3].token(), "my_header2.hpp");
    
	EXPECT_EQ(tokens[4].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, string_and_char_literals)
{
    constexpr string_view src = R"(
        "hello"
        'x'
        'y'"world"
    )";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 5u);

    EXPECT_EQ(tokens[0].type(), tt::string_literal);
    EXPECT_EQ(tokens[0].token(), "hello");

    EXPECT_EQ(tokens[1].type(), tt::character_literal);
    EXPECT_EQ(tokens[1].token(), "x");

    EXPECT_EQ(tokens[2].type(), tt::character_literal);
    EXPECT_EQ(tokens[2].token(), "y");

    EXPECT_EQ(tokens[3].type(), tt::string_literal);
    EXPECT_EQ(tokens[3].token(), "world");

    EXPECT_EQ(tokens[4].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, comments_are_skipped)
{
    constexpr string_view src =
        "foo//comment\n"
        "bar/* multi\n"
        "line*/baz";

    auto tokens = lex_all(src);

    //
    //for (auto& tok : tokens) {
    //    std::cout << "Type: " << to_str(tok.type()) << " Token: '" << tok.token() << "'" << std::endl; 
    //    if (tok.type() == tt::error) {
    //        std::cout << "Error: '" << tok.error_string() << "'" << std::endl;
    //    }
    //}

    ASSERT_EQ(tokens.size(), 4u);

    EXPECT_EQ(tokens[0].token(), "foo");
    EXPECT_EQ(tokens[1].token(), "bar");
    EXPECT_EQ(tokens[2].token(), "baz");

    EXPECT_EQ(tokens[3].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, line_and_column_numbers)
{
    const char* src =
        "foo\n"
        "  bar\n"
        "    123";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4u);

    EXPECT_EQ(tokens[0].token(), "foo");
    EXPECT_EQ(tokens[0].line_number(), 1u);
    EXPECT_EQ(tokens[0].column_number(), 1u);

    EXPECT_EQ(tokens[1].token(), "bar");
    EXPECT_EQ(tokens[1].line_number(), 2u);
    EXPECT_EQ(tokens[1].column_number(), 3u);

    EXPECT_EQ(tokens[2].token(), "123");
    EXPECT_EQ(tokens[2].line_number(), 3u);
    EXPECT_EQ(tokens[2].column_number(), 5u);

    EXPECT_EQ(tokens[3].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, unterminated_string_literal)
{
    const char* src = "\"unterminated \n   ";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2u);

    EXPECT_EQ(tokens[0].type(), tt::error);
    EXPECT_EQ(tokens[0].token(), "\"unterminated ");
    EXPECT_FALSE(tokens[0].error_string().empty());

    EXPECT_EQ(tokens[1].type(), tt::end_of_file);
}

TEST_F(preproc_lexer_test, punctuation_tokens)
{
    const char* src = "(){}[];,+-*/=!<>#";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 18u);

    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
		string_view expected_token(&src[i], 1u);
        EXPECT_EQ(tokens[i].type(), tt::punctuation);
        EXPECT_EQ(tokens[i].token(), expected_token);
        EXPECT_EQ(tokens[i].token().size(), 1u);
    }
}

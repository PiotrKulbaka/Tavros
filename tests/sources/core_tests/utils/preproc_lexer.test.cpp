#include <common.test.hpp>

#include <tavros/core/utils/preproc_lexer.hpp>

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
        case end_of_directive:  return "end_of_directive";
        case header_name:       return "header_name";
        case identifier:        return "identifier";
        case number:            return "number";
        case punctuation:       return "punctuation";
        case string_literal:    return "string_literal";
        case character_literal: return "character_literal";
        case error:             return "error";
        case end_of_source:     return "end_of_source";
    }

    return "<unknown token_type>";
}

void print_tokens(const std::vector<preproc_token>& tokens)
{
    for (auto& tok : tokens) {
        std::cout << "TokenType: " << to_str(tok.type()) << std::endl; 
        if (tok.type() != tt::end_of_source && tok.type() != tt::end_of_directive && tt::error != tok.type()) {
            std::cout << "  TokenStr: '" << tok.token() << "'" << std::endl; 
        }
        if (tok.type() == tt::error) {
            std::cout << "  TokenError: '" << tok.error_string() << "'" << std::endl;
        }
    }
}

static std::vector<preproc_token> lex_all(string_view src)
{
    preproc_lexer lexer(src.data(), src.data() + src.size());

    std::vector<preproc_token> tokens;
    for (;;)
    {
        auto tok = lexer.next_token();
        tokens.push_back(tok);

        if (tok.type() == tt::end_of_source)
        {
            break;
        }
    }
    return tokens;
}

TEST_F(preproc_lexer_test, empty_source)
{
    auto tokens = lex_all("");

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, identifiers_and_numbers)
{
    constexpr string_view src = "foo bar123 42";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), tt::identifier);
    EXPECT_EQ(tokens[0].token(), "foo");

    EXPECT_EQ(tokens[1].type(), tt::identifier);
    EXPECT_EQ(tokens[1].token(), "bar123");

    EXPECT_EQ(tokens[2].type(), tt::number);
    EXPECT_EQ(tokens[2].token(), "42");

    EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, identifiers_and_numbers2)
{
    constexpr string_view src = "/*z\n\nc*/foo/*abc*/bar123/*cde*///asd\n42";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), tt::identifier);
    EXPECT_EQ(tokens[0].token(), "foo");

    EXPECT_EQ(tokens[1].type(), tt::identifier);
    EXPECT_EQ(tokens[1].token(), "bar123");

    EXPECT_EQ(tokens[2].type(), tt::number);
    EXPECT_EQ(tokens[2].token(), "42");

    EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, preproc_directives)
{
    constexpr string_view src = R"(
        #include <some/path.h>   
        #    include   <  some/path2.h  >   
        #define FOO 123    
        #if __ABCD__    
    )";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 14);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");

    EXPECT_EQ(tokens[1].type(), tt::header_name);
    EXPECT_EQ(tokens[1].token(), "some/path.h");

    EXPECT_EQ(tokens[2].type(), tt::end_of_directive);
    EXPECT_TRUE(tokens[2].token().empty());

    EXPECT_EQ(tokens[3].type(), tt::directive);
    EXPECT_EQ(tokens[3].token(), "include");

    EXPECT_EQ(tokens[4].type(), tt::header_name);
    EXPECT_EQ(tokens[4].token(), "  some/path2.h  ");

    EXPECT_EQ(tokens[5].type(), tt::end_of_directive);
    EXPECT_TRUE(tokens[5].token().empty());

    EXPECT_EQ(tokens[6].type(), tt::directive);
    EXPECT_EQ(tokens[6].token(), "define");

    EXPECT_EQ(tokens[7].type(), tt::identifier);
    EXPECT_EQ(tokens[7].token(), "FOO");

    EXPECT_EQ(tokens[8].type(), tt::number);
    EXPECT_EQ(tokens[8].token(), "123");

    EXPECT_EQ(tokens[9].type(), tt::end_of_directive);
    EXPECT_TRUE(tokens[9].token().empty());

    EXPECT_EQ(tokens[10].type(), tt::directive);
    EXPECT_EQ(tokens[10].token(), "if");

    EXPECT_EQ(tokens[11].type(), tt::identifier);
    EXPECT_EQ(tokens[11].token(), "__ABCD__");

    EXPECT_EQ(tokens[12].type(), tt::end_of_directive);
    EXPECT_TRUE(tokens[12].token().empty());

    EXPECT_EQ(tokens[13].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}


TEST_F(preproc_lexer_test, quoted_header_name)
{
    constexpr string_view src = "#   include      \"my_header.hpp\"";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");
    
    EXPECT_EQ(tokens[1].type(), tt::header_name);
    EXPECT_EQ(tokens[1].token(), "my_header.hpp");
    
    EXPECT_EQ(tokens[2].type(), tt::end_of_directive);
    EXPECT_TRUE(tokens[2].token().empty());
    
	EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, comments_between_directive)
{
    constexpr string_view src =
        "/*comment*/#/*asd*/include/*comment2*/<my_header.hpp>//abc";

    auto tokens = lex_all(src);
    
    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");
    
    EXPECT_EQ(tokens[1].type(), tt::header_name);
    EXPECT_EQ(tokens[1].token(), "my_header.hpp");
    
    EXPECT_EQ(tokens[2].type(), tt::end_of_directive);

	EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}


TEST_F(preproc_lexer_test, comments_between_directive2)
{
    constexpr string_view src =
        "  /*com\nment*/#/**/include/*comment2*/<my_header.hpp>/*com\nment3*/   ";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");
    
    EXPECT_EQ(tokens[1].type(), tt::header_name);
    EXPECT_EQ(tokens[1].token(), "my_header.hpp");
    
    EXPECT_EQ(tokens[2].type(), tt::end_of_directive);

	EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, comment_inside_header_name)
{
    constexpr string_view src =
        "#include\"my/*comment*/header.hpp\"";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");

    EXPECT_EQ(tokens[1].type(), tt::header_name);
    EXPECT_EQ(tokens[1].token(), "my/*comment*/header.hpp");
    
    EXPECT_EQ(tokens[2].type(), tt::end_of_directive);

    EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, include_without_header_name)
{
    constexpr string_view src =
        "#include\n#include";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 7);

    EXPECT_EQ(tokens[0].type(), tt::directive);
    EXPECT_EQ(tokens[0].token(), "include");

    EXPECT_EQ(tokens[1].type(), tt::error);
    EXPECT_FALSE(tokens[1].error_string().empty());

    EXPECT_EQ(tokens[2].type(), tt::end_of_directive);
    EXPECT_TRUE(tokens[2].error_string().empty());

    EXPECT_EQ(tokens[3].type(), tt::directive);
    EXPECT_EQ(tokens[3].token(), "include");

    EXPECT_EQ(tokens[4].type(), tt::error);
    EXPECT_FALSE(tokens[4].error_string().empty());

    EXPECT_EQ(tokens[5].type(), tt::end_of_directive);
    EXPECT_TRUE(tokens[5].error_string().empty());

    EXPECT_EQ(tokens[6].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, hash_not_at_start_of_line)
{
    constexpr string_view src =
        "foo#include<file>";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 7);

    EXPECT_EQ(tokens[0].type(), tt::identifier);
    EXPECT_EQ(tokens[0].token(), "foo");
    EXPECT_EQ(tokens[0].col(), 1);

    EXPECT_EQ(tokens[1].type(), tt::punctuation);
    EXPECT_EQ(tokens[1].token(), "#");

    EXPECT_EQ(tokens[2].type(), tt::identifier);
    EXPECT_EQ(tokens[2].token(), "include");

    EXPECT_EQ(tokens[3].type(), tt::punctuation);
    EXPECT_EQ(tokens[3].token(), "<");

    EXPECT_EQ(tokens[4].type(), tt::identifier);
    EXPECT_EQ(tokens[4].token(), "file");

    EXPECT_EQ(tokens[5].type(), tt::punctuation);
    EXPECT_EQ(tokens[5].token(), ">");

    EXPECT_EQ(tokens[6].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, string_literal)
{
    constexpr string_view src = "\"hello\"";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::string_literal);
    EXPECT_EQ(tokens[0].token(), "hello");

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, string_literal_with_comments)
{
    constexpr string_view src = "/*asdf*/\"hel/*sadf*/lo\"//lkajsdflkj";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::string_literal);
    EXPECT_EQ(tokens[0].token(), "hel/*sadf*/lo");

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, unclosed_string_literal)
{
    constexpr string_view src = "/* 123 */ \"Hello world \n//asdf";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::error);
    EXPECT_EQ(tokens[0].line(), "/* 123 */ \"Hello world ");
    EXPECT_EQ(tokens[0].col(), 24);

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, string_literal_with_escaped_quote)
{
    constexpr string_view src = R"("hello \"world\" test")";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::string_literal);
    EXPECT_EQ(tokens[0].token(), "hello \\\"world\\\" test");

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);
    
    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, string_literal_with_escaped_close_quote)
{
    constexpr string_view src = R"( "hello\" )";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::error);
    EXPECT_EQ(tokens[0].line(), " \"hello\\\" ");
	EXPECT_FALSE(tokens[0].error_string().empty());
	EXPECT_EQ(tokens[0].col(), 11);
	EXPECT_EQ(tokens[0].row(), 1);

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);
    
    EXPECT_FALSE(assert_was_called());
}


TEST_F(preproc_lexer_test, two_unclosed_string_literals_and_identifier)
{
    constexpr string_view src = " \"hello\\\" foo // \n \"world\\\" bar /*asdf*/ \n /*asdf*/ foo //asdfd ";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), tt::error);
    EXPECT_EQ(tokens[0].line(), " \"hello\\\" foo // ");
	EXPECT_FALSE(tokens[0].error_string().empty());

    EXPECT_EQ(tokens[1].type(), tt::error);
    EXPECT_EQ(tokens[1].line(), " \"world\\\" bar /*asdf*/ ");
    EXPECT_FALSE(tokens[1].error_string().empty());

    EXPECT_EQ(tokens[2].type(), tt::identifier);
    EXPECT_EQ(tokens[2].token(), "foo");
    EXPECT_EQ(tokens[2].line(), " /*asdf*/ foo //asdfd ");
    EXPECT_TRUE(tokens[2].error_string().empty());

    EXPECT_EQ(tokens[3].type(), tt::end_of_source);
    
    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, character_literal)
{
    constexpr string_view src = "\'hello\'";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::character_literal);
    EXPECT_EQ(tokens[0].token(), "hello");

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, comments_are_skipped)
{
    constexpr string_view src =
        "foo//comment\n"
        "bar/* multi\n"
        "line*/baz";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].token(), "foo");
    EXPECT_EQ(tokens[0].line(), "foo//comment");
    EXPECT_TRUE(tokens[0].error_string().empty());
    EXPECT_EQ(tokens[0].col(), 1);
	EXPECT_EQ(tokens[0].row(), 1);


    EXPECT_EQ(tokens[1].token(), "bar");
    EXPECT_EQ(tokens[1].line(), "bar/* multi");
    EXPECT_TRUE(tokens[1].error_string().empty());
    EXPECT_EQ(tokens[1].col(), 1);
	EXPECT_EQ(tokens[1].row(), 2);

    EXPECT_EQ(tokens[2].token(), "baz");
    EXPECT_EQ(tokens[2].line(), "line*/baz");
    EXPECT_TRUE(tokens[2].error_string().empty());
    EXPECT_EQ(tokens[2].col(), 7);
	EXPECT_EQ(tokens[2].row(), 3);

    EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}


TEST_F(preproc_lexer_test, unclosed_comment_at_eos)
{
    constexpr string_view src = "   /* com\nment ";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_TRUE(tokens[0].token().empty());
    EXPECT_EQ(tokens[0].line(), "ment ");
    EXPECT_FALSE(tokens[0].error_string().empty());
    EXPECT_EQ(tokens[0].col(), 6);
	EXPECT_EQ(tokens[0].row(), 2);

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}


TEST_F(preproc_lexer_test, unclosed_comment_at_eos2)
{
    constexpr string_view src = "/*/";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::error);
    EXPECT_EQ(tokens[0].line(), "/*/");
    EXPECT_FALSE(tokens[0].error_string().empty());
    EXPECT_EQ(tokens[0].col(), 4);
	EXPECT_EQ(tokens[0].row(), 1);

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, comment_with_a_new_line)
{
    constexpr string_view src = " abc //def \\  \nqwe  \nzxc //comment\\\\\\\\";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 3);

    EXPECT_EQ(tokens[0].type(), tt::identifier);
    EXPECT_EQ(tokens[0].token(), "abc");

    EXPECT_EQ(tokens[1].type(), tt::identifier);
    EXPECT_EQ(tokens[1].token(), "zxc");

    EXPECT_EQ(tokens[2].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, line_and_column_numbers)
{
    const char* src =
        "foo\n"
        "  bar\n"
        "    123";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].token(), "foo");
    EXPECT_EQ(tokens[0].row(), 1);
    EXPECT_EQ(tokens[0].col(), 1);

    EXPECT_EQ(tokens[1].token(), "bar");
    EXPECT_EQ(tokens[1].row(), 2);
    EXPECT_EQ(tokens[1].col(), 3);

    EXPECT_EQ(tokens[2].token(), "123");
    EXPECT_EQ(tokens[2].row(), 3);
    EXPECT_EQ(tokens[2].col(), 5);

    EXPECT_EQ(tokens[3].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, unterminated_string_literal)
{
    const char* src = "  \"unterminated \n   ";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 2);

    EXPECT_EQ(tokens[0].type(), tt::error);
    EXPECT_EQ(tokens[0].token(), "\"unterminated ");
    EXPECT_FALSE(tokens[0].error_string().empty());

    EXPECT_EQ(tokens[1].type(), tt::end_of_source);

    EXPECT_FALSE(assert_was_called());
}

TEST_F(preproc_lexer_test, punctuation_tokens)
{
    const char* src = "(){}[];,+-*/=!<>#";

    auto tokens = lex_all(src);

    ASSERT_EQ(tokens.size(), 18);

    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
		string_view expected_token(&src[i], 1);
        EXPECT_EQ(tokens[i].type(), tt::punctuation);
        EXPECT_EQ(tokens[i].token(), expected_token);
        EXPECT_EQ(tokens[i].token().size(), 1);
    }

    EXPECT_FALSE(assert_was_called());
}

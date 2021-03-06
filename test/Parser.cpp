// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234

#ifdef _MSC_VER
#pragma warning(disable: 4503)
#endif

#define PARSER_LF_AS_CRLF
//#define PARSER_TEST_CORE_ONLY

#include "ParserIO.hpp"
#include "ParserCore.hpp"
#include <algorithm>

#ifndef PARSER_TEST_CORE_ONLY
#include "rfc5322/RFC5322Rules.hpp"
#include "rfc5234/RFC5324Rules.hpp"
#endif

#include <iostream>

#define TEST_RULE(type, name, str) \
{ \
    type nameResult; \
    auto parser(Make_ParserFromString(std::string(str))); \
    std::cout << "Parsing rule " << #name; \
    if (!ParseExact(parser, &nameResult, name())) \
        std::cout<< " failed" << std::endl; \
    else \
        std::cout << " succeed" << std::endl; \
}

#ifndef PARSER_TEST_CORE_ONLY

void TestRFC5234()
{
    using namespace RFC5234ABNF;
    TEST_RULE(SubstringPos, defined_as, "=/");
    TEST_RULE(SubstringPos, rulename, "defined-as");
    TEST_RULE(SubstringPos, comment, "; test comment\r\n");
    TEST_RULE(SubstringPos, c_nl, "; test comment\r\n");
    TEST_RULE(SubstringPos, c_wsp, "; test comment\r\n ");
    TEST_RULE(SubstringPos, defined_as, " ; test comment \r\n =");
    TEST_RULE(SubstringPos, defined_as, " ; test comment \r\n =/ ; test comment \r\n ");
    TEST_RULE(RepeatData, repeat, "1*5");
    TEST_RULE(RepeatData, repeat, "540");
    TEST_RULE(AlternationData, alternation, "rule1 / rule2 / rule3");
    TEST_RULE(ElementsData, elements, "*c-wsp");
    TEST_RULE(ElementsData, elements, "*c-wsp \"/\"");
    TEST_RULE(ElementsData, elements, "*c-wsp \"/\" *c-wsp concatenation");
    TEST_RULE(ElementsData, elements, "*(*c-wsp \"/\" *c-wsp concatenation)");
    TEST_RULE(ElementsData, elements, "concatenation\r\n *(*c-wsp \"/\" *c-wsp concatenation)");
    TEST_RULE(ElementsData, elements, "concatenation\r\n *(<test <\"string>)");
    TEST_RULE(RuleData, rule, "rule1 = concatenation\r\n *(<test <\"string>)\r\n");
    TEST_RULE(SubstringPos, c_nl, "\r\n");
    TEST_RULE(RuleData, rule, R"ABNF(defined-as     =  *c-wsp
)ABNF");
}

#include "rfc5234/ABNFParserGenerator.hpp"

void ParseABNF()
{
    std::string str = R"ABNF(
char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
                    ; quoted string of SP and VCHAR
                    ;  without DQUOTE
defined-as     =  *c-wsp ("=" / "=/") *c-wsp
                    ; basic rules definition and
                    ;  incremental alternatives

elements       =  alternation *c-wsp

c-wsp          =  WSP / (c-nl WSP)

c-nl           =  comment / CRLF
                    ; comment or newline

comment        =  ";" *(WSP / VCHAR) CRLF

alternation    =  concatenation
                *(*c-wsp "/" *c-wsp concatenation)

concatenation  =  repetition *(1*c-wsp repetition)

repetition     =  [repeat] element

repeat         =  1*DIGIT / (*DIGIT "*" *DIGIT)
repeat =/ DIGIT ; Added to test =/

element        =  rulename / group / option /
                char-val / num-val / prose-val

group          =  "(" *c-wsp alternation *c-wsp ")"

option         =  "[" *c-wsp alternation *c-wsp "]"

char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
                    ; quoted string of SP and VCHAR
                    ;  without DQUOTE

num-val        =  "%" (bin-val / dec-val / hex-val)

bin-val        =  "b" 1*BIT
                [ 1*("." 1*BIT) / ("-" 1*BIT) ]
                    ; series of concatenated bit values
                    ;  or single ONEOF range

dec-val        =  "d" 1*DIGIT
                [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]

hex-val        =  "x" 1*HEXDIG
                [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]

prose-val      =  "<" *(%x20-3D / %x3F-7E) ">"
                    ; bracketed string of SP and VCHAR
                    ;  without angles
                    ; prose description, to be used as
                    ;  last resort
)ABNF";
    using namespace RFC5234ABNF;

    //cout << CONSTANT(IsRawVariable(CharRange<0, 20>())) << std::endl;

    std::cout << "Parsing ABNF rules... ";

    auto parser(Make_ParserFromString(str));
    RuleListData rules;
    if (ParseExact(parser, &rules))
    {
        std::cout << "Success" << std::endl;
        GenerateABNFParser(std::cout, rules, parser.OutputBuffer());
    }
    else
    {
        std::cout << "Failure" << std::endl;
    }

    for (auto const & parseError : parser.Errors())
    {
        parseError(std::cerr, "");
    }

    return;
}

void TestRFC5322()
{
#if 0
#define TEST_RULE(type, name, str) \
    { \
        type nameResult; \
        auto parser(Make_ParserFromString(std::string(str))); \
        if (!Parse(parser, &nameResult, name()) || !parser.Ended()) \
            std::cout << "Parsing rule " << #name << " failed" << std::endl; \
        else \
            std::cout << "Parsing rule " << #name << " succeed" << std::endl; \
    }

    using namespace RFC5322;

    TEST_RULE(TextWithCommData, DotAtom, "local");
    TEST_RULE(AddrSpecData, AddrSpec, "local@domain");
    TEST_RULE(AngleAddrData, AngleAddr, "<local@domain> (comment)");
    TEST_RULE(TextWithCommData, Atom, "bllabla ");
    TEST_RULE(MultiTextWithCommData, DisplayName, "bllabla (comment) blabla");
    TEST_RULE(NameAddrData, NameAddr, "mrs johns <local@domain> (comment)");

    return;
#endif
}

void test_address(std::string const & addr)
{
    std::cout << addr;

    auto parser(Make_ParserFromString(addr));
    using namespace RFC5322;

    AddressListData addresses;
    if (ParseExact(parser, &addresses))
    {
        auto const & outBuffer = parser.OutputBuffer();
        std::cout << " is OK\n";
        std::cout << addresses.size() << " address" << (addresses.size() > 1 ? "es" : "") << " parsed." << std::endl;
        for (AddressData const & address : addresses)
        {
            auto displayAddress = [&](AddrSpecData const & addrSpec, auto indent)
            {
                std::cout << indent << "     Adress: " << std::endl;
                std::cout << indent << "       Local-Part: '" << ToString(outBuffer, false, addrSpec.LocalPart) << "'" << std::endl;
                std::cout << indent << "       Domain-Part: '" << ToString(outBuffer, false, addrSpec.DomainPart) << "'" << std::endl;
            };

            auto displayMailBox = [&](MailboxData const & mailbox, auto indent)
            {
                std::cout << indent << "   Mailbox:" << std::endl;
                if (IsEmpty(std::get<MailboxFields_AddrSpec>(mailbox)))
                {
                    auto const & nameAddrData = mailbox.NameAddr;
                    std::cout << indent << "     Display Name: '" << ToString(outBuffer, true, nameAddrData.DisplayName) << std::endl;
                    displayAddress(nameAddrData.Address.Content, indent);
                }
                else
                {
                    displayAddress(std::get<MailboxFields_AddrSpec>(mailbox), indent);
                }
            };

            if (false == IsEmpty(std::get<AddressFields_Mailbox>(address)))
            {
                displayMailBox(std::get<AddressFields_Mailbox>(address), "");
            }
            else
            {
                auto const & group = std::get<AddressFields_Group>(address);
                std::cout << "   Group:" << std::endl;
                std::cout << "     Display Name: '" << ToString(outBuffer, true, std::get<GroupFields_DisplayName>(group)) << "'" << std::endl;
                std::cout << "     Members:" << std::endl;
                for (auto const & groupAddr : std::get<GroupListFields_Mailboxes>(std::get<GroupFields_GroupList>(group)))
                {
                    displayMailBox(groupAddr, "   ");
                }
            }
        }
    }
    else
    {
        std::cout << " is NOK\n";
    }
    
    //for (auto const & parseError : parser.Errors())
    //{
    //    std::cerr << "Parsing error at " << std::get<0>(parseError) << ":" << std::get<1>(parseError) << std::endl;
    //}

    std::cout << std::endl;
}

#endif

template <typename TYPE>
class ToPrimitiveClass
{
public:
    using type = TYPE_F(ToPrimitive, TYPE);
};


PARSER_RULE_FORWARD(CContent)

PARSER_RULE(Comment, Repeat(CContent()))

// ccontent        =   ctext / quoted-pair / comment
PARSER_RULE_PARTIAL(CContent, Alternatives(RFC5234Core::ALPHA(), Comment()))

template <typename RULE>
class DataForRule;

template <int CH1, int CH2>
class DataForRule<CharRange<CH1, CH2> >
{
public:
    using type = SubstringPos;
};

template <size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE_OR_RULE>
class DataForRule<RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE_OR_RULE> >
{
public:
    using type = typename Impl::RepeatDataType<typename DataForRule<PRIMITIVE_OR_RULE>::type>::type;
};

class DataCContent;

template <>
class DataForRule<CContent>
{
public:
    using type = DataCContent;
};

class DataComment : public DataForRule<decltype(Repeat(CContent()))>::type
{
};

template <>
class DataForRule<Comment>
{
public:
    using type = DataComment;
};

class DataALPHA : public DataForRule<decltype(CharRange<0x41, 0x5A>())>::type
{
};

template <>
class DataForRule<RFC5234Core::ALPHA>
{
public:
    using type = DataALPHA;
};

namespace Impl
{
    template <typename... TYPES>
    class SeqDataType2;

    template <typename TYPE1, typename... OTHER_TYPES>
    class SeqDataType2<TYPE1, OTHER_TYPES...>
    {
    public:
        using type = decltype(Cat(
            std::declval<typename DataForRule<TYPE1>::type>(),
            std::declval<typename SeqDataType2<OTHER_TYPES...>::type>()));
    };
    
    template <typename TYPE1, typename TYPE2>
    class SeqDataType2<TYPE1, TYPE2>
    {
    public:
        using type = decltype(Cat(
            std::declval<typename DataForRule<TYPE1>::type>(),
            std::declval<typename DataForRule<TYPE2>::type>()));
    };

    template <typename TYPE1>
    class SeqDataType2<TYPE1>
    {
    public:
        //using type = DATATYPEFOR(TYPE1);
        using type = typename DataForRule<TYPE1>::type;
    };
}

template <typename SEQ_TYPE, typename... PRIMITIVES>
class DataForRule<SequenceType<SEQ_TYPE, PRIMITIVES...> >
{
public:
    using type = typename Impl::Detuplify<
        typename Impl::SeqDataType2<PRIMITIVES...>::type
    >::type;
};

class DataCContent : public DataForRule<decltype(Alternatives(RFC5234Core::ALPHA(), Comment()))>
{
};

void Test_Parser_Core()
{
    using namespace RFC5234Core;
    using namespace RFC5234ABNF;
//    using namespace RFC5322;

    //DataTypeFor<typename ToPrimitiveClass<ALPHA>::type> a0;
    //typename DataTypeFor<typename ToPrimitiveClass<ALPHA>::type>::Resolve<ToPrimitiveClass>::type a1;
    //typename DataTypeFor<typename ToPrimitiveClass<RepeatType<0, 1000, ALPHA> >::type>::template Resolve<ToPrimitiveClass>::type a2;
    //typename DataTypeFor<typename ToPrimitiveClass<repeat>::type>::template Resolve<ToPrimitiveClass>::type a3;
    //typename DataTypeFor<typename ToPrimitiveClass<RFC5322::Comment>::type>::template Resolve<ToPrimitiveClass>::type a4;
    //decltype(Resolve(LF())) toto2;

    //DataCContent test;
    //DataComment test2;

    //decltype(DefaultDataResolve(TypeBox<int>())) toto3;
    //decltype(DefaultDataResolve(Resolve(CR()))) toto4;
    //decltype(Resolve(CharVal<0x0D>())) toto5;
    //decltype(DefaultDataResolve(Resolve(CharVal<0x0D>()))) toto6;
    //DATATYPEFOR(CRLF) data;

//    typename DataType<RFC5322::Comment>::type data2 = {};
//    __debugbreak();
    return;
}

int main()
{
    Test_Parser_Core();

#ifndef PARSER_TEST_CORE_ONLY
    //TestRFC5322();
    //TestRFC5234();

    ParseABNF();

#endif

    return 0;
}
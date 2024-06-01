from typing import List

class DiagnosticKind:
    @staticmethod
    def source_buffer() -> List[str]:
        return [
            'ErrorOpeningFile',
            'ErrorStattingFile',
            'FileTooLarge',
            'ErrorReadingFile'
        ]
    
    @staticmethod
    def lexer() -> List[str]:
        return [
            "BinaryRealLiteral",
            "ContentBeforeStringTerminator",
            "DecimalEscapeSequence",
            "EmptyDigitSequence",
            "HexadecimalEscapeMissingDigits",
            "HexadecimalEscapeNotValid",
            "InvalidDigit",
            "InvalidDigitSeparator",
            "InvalidHorizontalWhitespaceInString",
            "IrregularDigitSeparators",
            "MismatchedClosing",
            "MismatchedIndentInString",
            "MultiLineStringWithDoubleQuotes",
            "NoWhitespaceAfterCommentIntroducer",
            "OctalRealLiteral",
            "TooManyDigits",
            "TrailingComment",
            "UnicodeEscapeInvalidDigits",
            "UnicodeEscapeMissingOpeningBrace",
            "UnicodeEscapeMissingClosingBrace",
            "UnicodeEscapeMissingBracedDigits",
            "UnicodeEscapeSurrogate",
            "UnicodeEscapeDigitsTooLarge",
            "UnicodeEscapeTooLarge",
            "UnknownBaseSpecifier",
            "UnknownEscapeSequence",
            "UnmatchedClosing",
            "UnrecognizedCharacters",
            "UnterminatedString",
            "WrongRealLiteralExponent",
        ]
    
    @staticmethod
    def test() -> List[str]:
        return [
            "TestDiagnostic",
            "TestDiagnosticNote",
            "TestDiagnosticWarning",
            "TestDiagnosticError",
            "TestDiagnosticInfo",
        ]
    
    
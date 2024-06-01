from dataclasses import Field, dataclass
from functools import cmp_to_key
from typing import List, Optional

@dataclass(frozen=True, slots=True, kw_only=True)
class Token:
    kind: str
    spelling: Optional[str] = None
    is_virtual: bool = False # True if it has one extra node in the AST
    is_symbol: bool = False # True if it is a symbol
    is_keyword: bool = False
    is_group: bool = False
    matching_kind: Optional[str] = None # The kind of the token that matches this token
    is_open: bool = False

    @property
    def snake_case_name(self) -> str:
        res = ''
        for i, c in enumerate(self.kind):
            if c.isupper() and i != 0:
                res += '_'
            res += c.lower()
        return res

    @staticmethod
    def compare_key(lhs: 'Token', rhs: 'Token') -> int:
        if lhs.spelling is None and rhs.spelling is None:
            return lhs.kind < rhs.kind
        
        if lhs.spelling is None:
            return -1
        
        if rhs.spelling is None:
            return 1
        
        l_len = len(lhs.spelling)
        r_len = len(rhs.spelling)
        if l_len != r_len:
            return r_len - l_len
        return lhs.spelling >= rhs.spelling

def validate_no_duplicate_kinds(tokens: Token) -> None:
    kinds = set()
    for token in tokens:
        if token.kind in kinds:
            raise ValueError(f'Duplicate token kind: {token.kind}')
        kinds.add(token.kind)

def sort_tokens(tokens: List[Token]):
    tokens.sort(key=cmp_to_key(Token.compare_key))

EBNF_TOKENS = [
    # Symbols
    Token(kind='Or', spelling='|', is_symbol=True),
    Token(kind='ZeroOrMore', spelling='*', is_symbol=True),
    Token(kind='OneOrMore', spelling='+', is_symbol=True),
    Token(kind='Optional', spelling='?', is_symbol=True),
    Token(kind='Concat', spelling=',', is_symbol=True),
    Token(kind='Semicolon', spelling=';', is_symbol=True),
    Token(kind='Colon', spelling=':', is_symbol=True),
    Token(kind='Equal', spelling='=', is_symbol=True),
    Token(kind='EqualColonColon', spelling='=::', is_symbol=True),
    Token(kind='Hash', spelling='#', is_symbol=True),
    Token(kind='Dollar', spelling='$', is_symbol=True),
    Token(kind='Range', spelling='..', is_symbol=True),
    Token(kind='RangeInclusive', spelling='..=', is_symbol=True),

    # Keywords
    Token(kind='Import', spelling='import', is_keyword=True)
]

LANG_TOKENS = [
    # Group
    Token(kind='OpenParen', spelling='(', is_group=True, matching_kind='CloseParen', is_open=True),
    Token(kind='CloseParen', spelling=')', is_group=True, matching_kind='OpenParen'),
    Token(kind='OpenBrace', spelling='{', is_group=True, matching_kind='CloseBrace', is_open=True),
    Token(kind='CloseBrace', spelling='}', is_group=True, matching_kind='OpenBrace'),
    Token(kind='OpenBracket', spelling='[', is_group=True, matching_kind='CloseBracket', is_open=True),
    Token(kind='CloseBracket', spelling=']', is_group=True, matching_kind='OpenBracket'),

    # Others
    Token(kind='Identifier'),
    Token(kind='RealLiteral'),
    Token(kind='IntegerLiteral'),
    Token(kind='StringLiteral'),
    Token(kind='CharacterLiteral'),
    Token(kind='BooleanLiteral'),
    Token(kind='FileStart'),
    Token(kind='FileEnd'),
]

class TokensBag:
    __slots__ = 'symbols', 'keywords', 'open_groups', 'close_groups', 'virtuals', 'misc'
    def __init__(self, tokens: List[Token]) -> None:
        validate_no_duplicate_kinds(tokens)
        sort_tokens(tokens)

        self.symbols: List[Token] = []
        self.keywords: List[Token] = []
        self.open_groups: List[Token] = []
        self.close_groups: List[Token] = []
        self.virtuals: List[Token] = []
        self.misc: List[Token] = []

        for token in tokens:
            if token.is_symbol:
                self.symbols.append(token)
            elif token.is_keyword:
                self.keywords.append(token)
            elif token.is_group and token.is_open:
                self.open_groups.append(token)
            elif token.is_group and not token.is_open:
                self.close_groups.append(token)
            elif token.is_virtual:
                self.virtuals.append(token)
            else:
                self.misc.append(token)
    
def make_ebnf_tokens_bag() -> TokensBag:
    return TokensBag(EBNF_TOKENS)

def make_lang_tokens_bag() -> TokensBag:
    return TokensBag(LANG_TOKENS)
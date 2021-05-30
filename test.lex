// imaginary code generator
%nocase
%func GetCookieValue
name=[A-Za-z][A-Za-z0-9]*
    {
    string name($start, $end);
    }
white=[ \t]*
value=[^ \t\r\n]+
cookie : name '=' value
    {
    string name($name.start, $name.end);
    // same as string name($name, $name.end);
    // same as string name($name, $name.len);
    string value($value.start, $value.end);
    $$ = TCookie(name,value);
    }
     
# Cookie[ \t]*:[ \t]*[A-Za-z]+=\([^ \t\r\n]+\)
#Cookie[ \t]*:[ \t]*`name`=\([^ \t\r\n]+\)
#     : 'Cookie' white ':' white name '=' value (';' name '=' value)*
: '^Cookie' white ':' white cookie (';' cookie)*
// same as : ^ 'Cookie' white ':' white cookie (';' cookie)*
/\*.*\*/
/[*].*[*]/
endcomment = "*/"
comment_text = .*&!{end_comment}
commnet_text = ([^*]*)|(\*!/[/])
comment : "/*" comment_text "*/"

notstar   [^*]|\n
noteoc    "*"!/"/"
comment_text = ({noteoc}|{notstar})*
comment = "/*"{comment_text}"*/"

balpar : '(' balpar ')'
       |  'x'
// match a 6-letter word containing "cat"

hascat
    : (/[a-z]{6})([a-z]*"cat"[a-z]*)

=========================================
%name GetHttpCookies
%lang C++

%case off // case-insensitive matches, please

// regular definitions
dq      = "\""
ws      = [ \t]
eol     = \r?\n
name    = [A-Za-z]+[A-Za-z]*
// from RFC 6265
value   = [^\x21\x23-\x2B\x2D-\x3A\x3C-\x5B\x5D-\x7E]*


CookieHeader
    : eol Cookie ws* ':' ws* Cookie (';' Cookie)* eol
    ;

Cookie
    : ws* name '=' value ws*
        {
        name  = $name;
        value = $value;
        }
    ;

CookieValue
    : value
        { $$ = $value; }
    | dq value dq
        { $$ = $value; }
    ;

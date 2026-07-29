#include "md_highlight.h"
#include <string.h>
#include <ctype.h>

/* ---------------- output helpers ---------------- */

struct hl_out {
	struct morph_md_sbuf *destination;
};

static void hl_out_init(struct hl_out *out, struct morph_md_sbuf *destination)
{
	out->destination = destination;
}

static void hl_out_n(struct hl_out *out, const char *text, size_t len)
{
	morph_md_sbuf_append_n(out->destination, text, len);
}

static void hl_out_s(struct hl_out *o, const char *s)
{
	hl_out_n(o, s, strlen(s));
}

static void hl_out_ch(struct hl_out *o, char c)
{
	hl_out_n(o, &c, 1);
}

/* ---------------- ANSI color codes ---------------- */

#define HL_RESET   "\033[0m"
#define HL_DEFAULT "\033[38;5;250m"
#define HL_KW      "\033[1;33m"
#define HL_TYPE    "\033[36m"
#define HL_BUILTIN "\033[34m"
#define HL_STR     "\033[32m"
#define HL_CHR     "\033[32m"
#define HL_CMT     "\033[2;36m"
#define HL_DOC     "\033[36m"
#define HL_NUM     "\033[35m"
#define HL_PREP    "\033[35m"
#define HL_FUNC    "\033[37m"

/* ---------------- token types ---------------- */

enum hl_tok {
	HL_TOK_DEFAULT = 0,
	HL_TOK_KEYWORD,
	HL_TOK_TYPE,
	HL_TOK_BUILTIN,
	HL_TOK_STRING,
	HL_TOK_CHAR,
	HL_TOK_COMMENT,
	HL_TOK_DOC_COMMENT,
	HL_TOK_NUMBER,
	HL_TOK_PREPROC,
	HL_TOK_FUNC,
};

/* ---------------- language definition ---------------- */

struct lang_def {
	const char *name;
	const char *const *keywords;
	const char *const *types;
	const char *const *builtins;
	char comment_line;
	char comment_multi_s;
	char comment_multi_m;
	char comment_multi_e1;
	char comment_multi_e2;
	char string_char;
	char string_char2;
	char char_char;
	int has_preproc;
	int has_func_call;
};

/* ---------------- keyword tables ---------------- */

static const char *const kw_c[] = {
	"auto", "break", "case", "const", "continue", "default", "do", "else", "enum",
	"extern", "for", "goto", "if", "inline", "register", "restrict", "return", "sizeof",
	"static", "struct", "switch", "typedef", "union", "volatile", "while", "_Alignas",
	"_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary", "_Noreturn",
	"_Static_assert", "_Thread_local",
	NULL,
};

static const char *const tp_c[] = {
	"void", "char", "short", "int", "long", "float", "double", "signed", "unsigned",
	"size_t", "ssize_t", "ptrdiff_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
	"int8_t", "int16_t", "int32_t", "int64_t", "bool",
	NULL,
};

static const char *const bi_c[] = {
	"NULL", "EOF", "stdin", "stdout", "stderr", "EXIT_SUCCESS", "EXIT_FAILURE", "BUFSIZ",
	"FILENAME_MAX", "FOPEN_MAX", "RAND_MAX", "CHAR_BIT", "CHAR_MIN", "CHAR_MAX", "INT_MIN",
	"INT_MAX", "LONG_MIN", "LONG_MAX", "UINT_MAX", "ULONG_MAX", "FLT_MIN", "FLT_MAX",
	"DBL_MIN", "DBL_MAX", "LDBL_MIN", "LDBL_MAX", "CLOCKS_PER_SEC",
	NULL,
};

static const char *const kw_cpp[] = {
	"alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool",
	"break", "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class",
	"compl", "concept", "const", "consteval", "constexpr", "const_cast", "continue",
	"co_await", "co_return", "co_yield", "decltype", "default", "delete", "do", "dynamic_cast",
	"else", "enum", "explicit", "export", "extern", "false", "for", "friend", "goto",
	"if", "inline", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
	"operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
	"requires", "return", "signed", "sizeof", "static", "static_assert", "static_cast",
	"struct", "switch", "template", "this", "thread_local", "throw", "true", "try",
	"typedef", "typeid", "typename", "union", "using", "virtual", "volatile", "while",
	"xor", "xor_eq", "override", "final",
	NULL,
};

static const char *const tp_cpp[] = {
	"void", "short", "int", "long", "float", "double", "unsigned", "size_t", "ssize_t",
	"ptrdiff_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t",
	"int32_t", "int64_t", "string", "wstring", "u16string", "u32string", "string_view",
	"vector", "map", "set", "unordered_map", "unordered_set", "array", "deque", "list",
	"forward_list", "queue", "stack", "pair", "tuple", "optional", "variant", "any",
	"unique_ptr", "shared_ptr", "weak_ptr",
	NULL,
};

static const char *const bi_cpp[] = {
	"NULL", "nullptr", "cout", "cin", "cerr", "clog", "endl", "flush", "std", "this",
	NULL,
};

static const char *const kw_java[] = {
	"abstract", "assert", "break", "case", "catch", "class", "const", "continue",
	"default", "do", "else", "enum", "extends", "final", "finally", "for", "goto",
	"if", "implements", "import", "instanceof", "interface", "native", "new", "package",
	"private", "protected", "public", "return", "static", "strictfp", "super", "switch",
	"synchronized", "this", "throw", "throws", "transient", "try", "volatile", "while",
	"true", "false", "null",
	NULL,
};

static const char *const tp_java[] = {
	"boolean", "byte", "char", "double", "float", "int", "long", "short", "void",
	"String", "Integer", "Long", "Double", "Float", "Boolean", "Byte", "Short", "Character",
	"Object", "Class", "Thread", "Runnable", "Exception", "RuntimeException", "ArrayList",
	"HashMap", "HashSet", "LinkedList", "TreeMap", "TreeSet", "Vector", "Hashtable",
	"Properties", "Arrays", "Collections", "Iterator", "List", "Map", "Set", "Queue",
	"Deque", "Comparable", "Comparator", "Optional", "Stream",
	NULL,
};

static const char *const bi_java[] = {
	"System", "Math", "Runtime", "Process", "ProcessBuilder", "this", "super",
	NULL,
};

static const char *const kw_python[] = {
	"False", "None", "True", "and", "as", "assert", "async", "await", "break", "class",
	"continue", "def", "del", "elif", "else", "except", "finally", "for", "from",
	"global", "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass",
	"raise", "return", "try", "while", "with", "yield",
	NULL,
};

static const char *const tp_python[] = {
	"int", "float", "complex", "str", "bytes", "bool", "list", "tuple", "dict", "set",
	"frozenset", "range", "type", "object", "NoneType", "bytearray", "memoryview",
	"enumerate", "zip", "map", "filter", "reversed", "sorted", "slice", "property",
	"classmethod", "staticmethod", "super",
	NULL,
};

static const char *const bi_python[] = {
	"print", "input", "len", "range", "open", "close", "read", "write", "append",
	"extend", "insert", "remove", "pop", "sort", "reverse", "keys", "values", "items",
	"get", "update", "abs", "max", "min", "sum", "round", "isinstance", "issubclass",
	"hasattr", "getattr", "setattr", "delattr", "callable", "iter", "next", "id",
	"hash", "dir", "vars", "globals", "locals", "exec", "eval", "compile", "repr",
	"format", "ord", "chr", "hex", "oct", "bin", "pow", "divmod", "all", "any", "self",
	"cls",
	NULL,
};

static const char *const kw_rust[] = {
	"as", "async", "await", "break", "const", "continue", "crate", "dyn", "else",
	"enum", "extern", "fn", "for", "if", "impl", "in", "let", "loop", "match", "mod",
	"move", "mut", "pub", "ref", "return", "self", "Self", "static", "struct", "super",
	"trait", "type", "unsafe", "use", "where", "while", "true", "false",
	NULL,
};

static const char *const tp_rust[] = {
	"i8", "i16", "i32", "i64", "i128", "isize", "u8", "u16", "u32", "u64", "u128",
	"usize", "f32", "f64", "bool", "char", "str", "String", "Vec", "Box", "Rc", "Arc",
	"Option", "Result", "Ok", "Err", "Some", "None", "HashMap", "HashSet", "BTreeMap",
	"BTreeSet", "Cow", "Cell", "RefCell", "Mutex", "RwLock", "Duration", "Instant",
	"Path", "PathBuf", "Range", "RangeInclusive", "Iterator", "IntoIterator", "FromIterator",
	"Display", "Debug", "Clone", "Copy", "Default", "Eq", "PartialEq", "Ord", "PartialOrd",
	"Hash", "Send", "Sync", "Sized", "Unsize",
	NULL,
};

static const char *const bi_rust[] = {
	"println", "print", "eprintln", "eprint", "format", "panic", "assert", "assert_eq",
	"assert_ne", "todo", "unimplemented", "unreachable", "vec", "box", "dbg", "clone",
	"to_string", "from", "into", "try_from", "try_into", "as_ref", "as_mut", "derive",
	"macro_rules", "println!", "print!", "eprintln!", "eprint!", "format!", "vec!",
	"assert!", "assert_eq!", "assert_ne!", "panic!", "todo!", "unimplemented!", "unreachable!",
	"dbg!", "write!", "writeln!", "concat!", "env!", "include!", "include_str!", "include_bytes!",
	"file!", "line!", "column!", "module_path!", "cfg!", "test!",
	NULL,
};

static const char *const kw_go[] = {
	"break", "case", "chan", "const", "continue", "default", "defer", "else", "fallthrough",
	"for", "func", "go", "goto", "if", "import", "interface", "map", "package", "range",
	"return", "select", "struct", "switch", "type", "var", "true", "false", "nil",
	NULL,
};

static const char *const tp_go[] = {
	"bool", "byte", "complex64", "complex128", "error", "float32", "float64", "int",
	"int8", "int16", "int32", "int64", "rune", "string", "uint", "uint8", "uint16",
	"uint32", "uint64", "uintptr", "any",
	NULL,
};

static const char *const bi_go[] = {
	"append", "cap", "close", "copy", "delete", "imag", "len", "make", "new", "panic",
	"print", "println", "real", "recover", "fmt", "os", "io", "http", "strings", "strconv",
	"json", "time", "context", "sync", "errors", "reflect", "sort", "math", "log",
	"net", "crypto",
	NULL,
};

static const char *const kw_js[] = {
	"async", "await", "break", "case", "catch", "class", "const", "continue", "debugger",
	"default", "delete", "do", "else", "export", "extends", "finally", "for", "function",
	"if", "import", "in", "instanceof", "let", "new", "of", "return", "static", "super",
	"switch", "this", "throw", "try", "typeof", "var", "void", "while", "with", "yield",
	"true", "false", "null", "undefined",
	NULL,
};

static const char *const tp_js[] = {
	"boolean", "number", "string", "object", "symbol", "bigint", "any", "never", "unknown",
	"void", "Array", "Promise", "Map", "Set", "WeakMap", "WeakSet", "Date", "RegExp",
	"Error", "TypeError", "RangeError", "SyntaxError", "Record", "Partial", "Required",
	"Readonly", "Pick", "Omit", "Exclude", "Extract", "ReturnType", "Parameters",
	"InstanceType",
	NULL,
};

static const char *const bi_js[] = {
	"console", "Math", "JSON", "Object", "Array", "String", "Number", "Boolean", "Symbol",
	"parseInt", "parseFloat", "isNaN", "isFinite", "encodeURI", "decodeURI", "encodeURIComponent",
	"decodeURIComponent", "setTimeout", "setInterval", "clearTimeout", "clearInterval",
	"requestAnimationFrame", "fetch", "alert", "confirm", "prompt", "document", "window",
	"globalThis", "process", "require", "module", "exports", "undefined", "NaN", "Infinity",
	NULL,
};

static const char *const kw_ts[] = {
	"abstract", "as", "async", "await", "break", "case", "catch", "class", "const",
	"constructor", "continue", "debugger", "default", "delete", "do", "else", "enum",
	"export", "extends", "finally", "for", "from", "function", "if", "implements",
	"import", "in", "instanceof", "interface", "is", "keyof", "let", "module", "namespace",
	"new", "of", "package", "private", "protected", "public", "readonly", "require",
	"return", "satisfies", "static", "super", "switch", "this", "throw", "try", "type",
	"typeof", "var", "void", "while", "with", "yield", "true", "false", "null", "undefined",
	NULL,
};

static const char *const kw_ruby[] = {
	"__ENCODING__", "__FILE__", "__LINE__", "alias", "and", "begin", "break", "case",
	"class", "def", "defined?", "do", "else", "elsif", "end", "ensure", "false", "for",
	"if", "in", "module", "next", "nil", "not", "or", "redo", "rescue", "retry", "return",
	"self", "super", "then", "true", "undef", "unless", "until", "when", "while",
	"yield",
	NULL,
};

static const char *const tp_ruby[] = {
	"Array", "Hash", "String", "Integer", "Float", "Symbol", "Regexp", "Range", "NilClass",
	"TrueClass", "FalseClass", "Numeric", "Proc", "Method", "Enumerator", "IO", "File",
	"Dir", "Time", "Date", "DateTime", "Exception", "StandardError", "Struct", "Set",
	"Matrix", "Vector", "Complex", "Rational", "BigDecimal",
	NULL,
};

static const char *const bi_ruby[] = {
	"require", "require_relative", "include", "extend", "attr_reader", "attr_writer",
	"attr_accessor", "puts", "print", "p", "pp", "raise", "fail", "catch", "throw",
	"loop", "lambda", "proc", "new", "freeze", "inspect", "to_s", "to_i", "to_f",
	"to_a", "to_h", "nil?", "empty?", "length", "size", "each", "map", "select", "reject",
	"reduce", "find", "detect", "any?", "all?", "none?", "one?", "count", "min", "max",
	"sort", "sort_by", "first", "last", "flatten", "compact", "uniq", "merge", "join",
	"split", "chomp", "strip", "gsub", "sub", "scan",
	NULL,
};

static const char *const kw_php[] = {
	"abstract", "and", "array", "as", "break", "callable", "case", "catch", "class",
	"clone", "const", "continue", "declare", "default", "die", "do", "echo", "else",
	"elseif", "empty", "eval", "exit", "extends", "final", "finally", "fn", "for",
	"foreach", "function", "global", "goto", "if", "implements", "include", "include_once",
	"instanceof", "insteadof", "interface", "isset", "list", "match", "namespace",
	"new", "or", "print", "private", "protected", "public", "require", "require_once",
	"return", "static", "switch", "throw", "trait", "try", "unset", "use", "var",
	"while", "xor", "yield", "true", "false", "null",
	NULL,
};

static const char *const tp_php[] = {
	"bool", "int", "float", "double", "string", "array", "object", "callable", "iterable",
	"void", "mixed", "never", "self", "parent", "static",
	NULL,
};

static const char *const bi_php[] = {
	"strlen", "strpos", "str_replace", "substr", "strtolower", "strtoupper", "trim",
	"explode", "implode", "sprintf", "printf", "var_dump", "print_r", "isset", "empty",
	"unset", "count", "in_array", "array_push", "array_pop", "array_merge", "array_keys",
	"array_values", "array_map", "array_filter", "sort", "json_encode", "json_decode",
	"file_get_contents", "file_put_contents", "fopen", "fclose", "fread", "fwrite",
	"fgets", "is_file", "is_dir", "mkdir", "rmdir", "unlink", "rename", "copy", "date",
	"time", "strtotime", "header", "session_start", "PDO",
	NULL,
};

static const char *const kw_swift[] = {
	"actor", "as", "async", "await", "break", "case", "catch", "class", "continue",
	"convenience", "default", "defer", "deinit", "do", "dynamic", "else", "enum",
	"extension", "fallthrough", "fileprivate", "final", "for", "func", "guard", "if",
	"import", "in", "indirect", "init", "inout", "internal", "is", "lazy", "let",
	"mutating", "nil", "nonisolated", "nonmutating", "open", "operator", "optional",
	"override", "package", "postfix", "prefix", "private", "protocol", "public", "repeat",
	"required", "rethrows", "return", "self", "Self", "some", "static", "struct",
	"subscript", "super", "switch", "throw", "throws", "try", "typealias", "var",
	"where", "while", "willSet", "didSet", "true", "false",
	NULL,
};

static const char *const tp_swift[] = {
	"Int", "Int8", "Int16", "Int32", "Int64", "UInt", "UInt8", "UInt16", "UInt32",
	"UInt64", "Float", "Double", "Bool", "String", "Character", "Array", "Dictionary",
	"Set", "Optional", "Result", "Any", "AnyObject", "Void", "Never", "Error", "URL",
	"Data", "Date", "UUID", "Range", "ClosedRange", "Comparable", "Equatable", "Hashable",
	"Codable", "Encodable", "Decodable", "Identifiable", "Observable",
	NULL,
};

static const char *const bi_swift[] = {
	"print", "debugPrint", "dump", "assert", "assertionFailure", "precondition", "fatalError",
	"type", "min", "max", "abs", "count", "isEmpty", "append", "remove", "insert",
	"contains", "filter", "map", "reduce", "compactMap", "flatMap", "sorted", "first",
	"last", "dropFirst", "dropLast", "prefix", "suffix", "split", "joined", "enumerated",
	"zip", "reversed", "shuffled", "forEach",
	NULL,
};

static const char *const kw_kotlin[] = {
	"abstract", "actual", "annotation", "as", "break", "by", "catch", "class", "companion",
	"const", "constructor", "continue", "crossinline", "data", "do", "else", "enum",
	"expect", "external", "false", "final", "finally", "for", "fun", "if", "in", "infix",
	"init", "inline", "inner", "interface", "internal", "is", "lateinit", "noinline",
	"null", "object", "open", "operator", "out", "override", "package", "private",
	"protected", "public", "reified", "return", "sealed", "suspend", "tailrec", "this",
	"throw", "true", "try", "typealias", "typeof", "val", "var", "vararg", "when",
	"where", "while",
	NULL,
};

static const char *const tp_kotlin[] = {
	"Byte", "Short", "Int", "Long", "Float", "Double", "Boolean", "Char", "String",
	"Array", "List", "Map", "Set", "MutableList", "MutableMap", "MutableSet", "Sequence",
	"Iterator", "Iterable", "Collection", "Unit", "Nothing", "Any", "Result", "Pair",
	"Triple", "Range", "ClosedRange", "Comparable", "Equatable",
	NULL,
};

static const char *const bi_kotlin[] = {
	"println", "print", "readLine", "arrayOf", "listOf", "mapOf", "setOf", "mutableListOf",
	"mutableMapOf", "mutableSetOf", "sequenceOf", "toInt", "toFloat", "toDouble",
	"toLong", "toShort", "toByte", "toChar", "toString", "size", "length", "isEmpty",
	"contains", "filter", "map", "forEach", "flatMap", "reduce", "fold", "sorted",
	"sortedBy", "first", "last", "count", "min", "max", "sum", "average", "joinToString",
	"with", "apply", "let", "run", "also", "takeIf", "takeUnless", "lazy",
	NULL,
};

static const char *const kw_scala[] = {
	"abstract", "case", "catch", "class", "def", "do", "else", "extends", "false",
	"final", "finally", "for", "forSome", "if", "implicit", "import", "lazy", "match",
	"new", "null", "object", "override", "package", "private", "protected", "return",
	"sealed", "super", "this", "throw", "trait", "true", "try", "type", "val", "var",
	"while", "with", "yield",
	NULL,
};

static const char *const tp_scala[] = {
	"Byte", "Short", "Int", "Long", "Float", "Double", "Boolean", "Char", "String",
	"Unit", "Nothing", "Any", "AnyRef", "AnyVal", "Null", "Option", "Some", "None",
	"Either", "Left", "Right", "Try", "Success", "Failure", "List", "Map", "Set",
	"Seq", "Vector", "Stream", "Iterator", "Array", "Tuple2", "Tuple3", "Future",
	"Await", "Duration", "ExecutionContext",
	NULL,
};

static const char *const bi_scala[] = {
	"println", "print", "printf", "readLine", "toInt", "toFloat", "toDouble", "toLong",
	"toString", "isEmpty", "nonEmpty", "length", "size", "head", "last", "tail", "init",
	"take", "drop", "map", "flatMap", "filter", "foreach", "foldLeft", "foldRight",
	"reduce", "zip", "unzip", "flatten", "mkString", "contains", "exists", "forall",
	"find", "count", "distinct", "sorted", "sortBy", "reverse", "groupBy", "collect",
	"scanLeft", "require", "assert", "assume", "identity", "const", "implicitly",
	NULL,
};

static const char *const kw_bash[] = {
	"if", "then", "else", "elif", "fi", "case", "esac", "for", "while", "until", "do",
	"done", "in", "function", "select", "time", "coproc", "return", "exit", "break",
	"continue", "declare", "export", "local", "readonly", "typeset", "unset", "source",
	"alias", "unalias", "set", "shift", "eval", "exec", "trap", "wait", "read", "readarray",
	"mapfile", "let", "true", "false", "test",
	NULL,
};

static const char *const bi_bash[] = {
	"echo", "printf", "cat", "ls", "cd", "pwd", "mkdir", "rm", "cp", "mv", "chmod",
	"find", "grep", "sed", "awk", "sort", "uniq", "wc", "head", "tail", "cut", "tr",
	"tee", "xargs", "basename", "dirname", "date", "sleep", "touch", "mktemp", "realpath",
	"readlink", "which", "type", "command", "builtin", "hash", "getopts", "pushd",
	"popd", "dirs", "jobs", "fg", "bg", "kill", "disown", "nohup", "nice", "shopt",
	"complete", "compgen", "compopt", "bind", "help", "enable",
	NULL,
};

static const char *const kw_shell[] = {
	"if", "then", "else", "elif", "fi", "case", "esac", "for", "while", "do", "done",
	"in", "function", "return", "exit", "break", "continue", "true", "false", "test",
	NULL,
};

static const char *const bi_shell[] = {
	"echo", "printf", "cd", "pwd", "ls", "mkdir", "rm", "cp", "mv", "cat", "grep",
	"sed", "awk", "sort", "uniq", "wc", "head", "tail", "cut", "tr", "test", "true",
	"false", "exit", "return", "export", "read", "shift",
	NULL,
};

static const char *const kw_sql[] = {
	"SELECT", "FROM", "WHERE", "INSERT", "INTO", "VALUES", "UPDATE", "SET", "DELETE",
	"CREATE", "TABLE", "ALTER", "DROP", "INDEX", "VIEW", "DATABASE", "SCHEMA", "TRIGGER",
	"FUNCTION", "PROCEDURE", "AS", "ON", "AND", "OR", "NOT", "IN", "IS", "NULL", "LIKE",
	"BETWEEN", "EXISTS", "DISTINCT", "GROUP", "BY", "HAVING", "ORDER", "ASC", "DESC",
	"LIMIT", "OFFSET", "JOIN", "INNER", "LEFT", "RIGHT", "FULL", "OUTER", "CROSS",
	"NATURAL", "UNION", "ALL", "EXCEPT", "INTERSECT", "CASE", "WHEN", "THEN", "ELSE",
	"END", "WITH", "RECURSIVE", "OVER", "PARTITION", "ROWS", "RANGE", "UNBOUNDED",
	"PRECEDING", "FOLLOWING", "CURRENT", "ROW", "FETCH", "NEXT", "ONLY", "PRIMARY",
	"KEY", "FOREIGN", "REFERENCES", "UNIQUE", "CHECK", "DEFAULT", "CONSTRAINT", "CASCADE",
	"RESTRICT", "GRANT", "REVOKE", "BEGIN", "COMMIT", "ROLLBACK", "TRANSACTION", "SAVEPOINT",
	"IF", "TEMPORARY", "TEMP", "GLOBAL", "LOCAL",
	NULL,
};

static const char *const tp_sql[] = {
	"INT", "INTEGER", "BIGINT", "SMALLINT", "TINYINT", "FLOAT", "DOUBLE", "DECIMAL",
	"NUMERIC", "REAL", "CHAR", "VARCHAR", "TEXT", "BLOB", "CLOB", "DATE", "TIME",
	"DATETIME", "TIMESTAMP", "BOOLEAN", "BIT", "BINARY", "VARBINARY", "SERIAL", "BIGSERIAL",
	"UUID", "JSON", "JSONB", "XML", "MONEY", "INTERVAL", "MACADDR", "INET", "CIDR",
	NULL,
};

static const char *const bi_sql[] = {
	"COUNT", "SUM", "AVG", "MIN", "MAX", "COALESCE", "NULLIF", "CAST", "CONVERT",
	"EXTRACT", "NOW", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP", "LOWER",
	"UPPER", "TRIM", "LENGTH", "SUBSTRING", "CONCAT", "REPLACE", "POSITION", "ABS",
	"CEIL", "FLOOR", "ROUND", "MOD", "POWER", "SQRT", "LN", "LOG", "EXP", "SIN", "COS",
	"TAN", "ASIN", "ACOS", "ATAN", "ATAN2", "RANK", "DENSE_RANK", "ROW_NUMBER", "LEAD",
	"LAG", "FIRST_VALUE", "LAST_VALUE", "NTH_VALUE", "NTILE", "ARRAY_AGG", "STRING_AGG",
	NULL,
};

static const char *const kw_lua[] = {
	"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto",
	"if", "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until",
	"while",
	NULL,
};

static const char *const tp_lua[] = {
	"nil", "boolean", "number", "string", "table", "function", "userdata", "thread",
	NULL,
};

static const char *const bi_lua[] = {
	"print", "type", "tostring", "tonumber", "pairs", "ipairs", "next", "select",
	"unpack", "require", "pcall", "xpcall", "error", "assert", "collectgarbage", "rawget",
	"rawset", "rawequal", "rawlen", "setmetatable", "getmetatable", "load", "loadstring",
	"dofile", "loadfile", "coroutine", "table", "string", "math", "io", "os", "debug",
	"package", "_G", "_VERSION", "self", "string.format", "string.find", "string.match",
	"string.gmatch", "string.gsub", "string.len", "string.sub", "string.rep", "string.reverse",
	"string.lower", "string.upper", "string.byte", "string.char", "table.insert",
	"table.remove", "table.sort", "table.concat", "math.abs", "math.floor", "math.ceil",
	"math.max", "math.min", "math.random", "math.sqrt", "math.sin", "math.cos", "math.tan",
	"math.exp", "math.log", "math.pi", "io.open", "io.read", "io.write", "io.close",
	"io.lines", "os.date", "os.time", "os.clock", "os.difftime",
	NULL,
};

static const char *const kw_perl[] = {
	"my", "our", "local", "state", "sub", "if", "elsif", "else", "unless", "while",
	"until", "for", "foreach", "do", "eval", "require", "use", "no", "package", "BEGIN",
	"END", "INIT", "CHECK", "die", "warn", "last", "next", "redo", "return", "goto",
	"default", "given", "when", "break", "say", "print",
	NULL,
};

static const char *const tp_perl[] = {
	"SCALAR", "ARRAY", "HASH", "CODE", "REF", "GLOB", "LVALUE", "IO", "FORMAT", "Regexp",
	"VSTRING",
	NULL,
};

static const char *const bi_perl[] = {
	"print", "say", "warn", "die", "exit", "open", "close", "read", "write", "seek",
	"tell", "eof", "binmode", "chomp", "chop", "chr", "ord", "lc", "uc", "length",
	"substr", "index", "rindex", "split", "join", "reverse", "sort", "grep", "map",
	"push", "pop", "shift", "unshift", "splice", "keys", "values", "each", "exists",
	"delete", "defined", "undef", "ref", "bless", "tie", "tied", "abs", "int", "rand",
	"srand", "sqrt", "exp", "log", "sin", "cos", "atan2", "localtime", "gmtime", "time",
	"sleep", "alarm", "fork", "exec", "system", "wait", "waitpid", "kill", "chdir",
	"chmod", "chown", "mkdir", "rmdir", "unlink", "rename", "link", "symlink", "readlink",
	"stat", "lstat", "glob", "opendir", "readdir", "closedir", "scalar", "wantarray",
	"caller", "prototype",
	NULL,
};

static const char *const kw_haskell[] = {
	"module", "where", "import", "data", "type", "newtype", "class", "instance", "deriving",
	"if", "then", "else", "case", "of", "let", "in", "do", "mdo", "forall", "family",
	"infix", "infixl", "infixr", "qualified", "as", "hiding", "default", "foreign",
	"export", "safe", "unsafe", "interruptible", "ccall", "stdcall", "javascript",
	"capi", "prim",
	NULL,
};

static const char *const tp_haskell[] = {
	"Int", "Integer", "Float", "Double", "Char", "Bool", "String", "IO", "Maybe",
	"Just", "Nothing", "Either", "Left", "Right", "Ordering", "LT", "EQ", "GT", "Read",
	"Show", "Eq", "Ord", "Enum", "Bounded", "Num", "Real", "Integral", "Fractional",
	"Floating", "RealFrac", "RealFloat", "Functor", "Applicative", "Monad", "Monoid",
	"Foldable", "Traversable", "Semigroup", "Alternative", "MonadPlus", "MonadIO",
	"MonadFail", "Generic", "Typeable", "Data", "Storable", "ForeignPtr", "Ptr", "StablePtr",
	"Handle", "FilePath", "IOException",
	NULL,
};

static const char *const bi_haskell[] = {
	"putStrLn", "putStr", "print", "getLine", "getContents", "interact", "readFile",
	"writeFile", "appendFile", "readIO", "readLn", "map", "filter", "foldl", "foldr",
	"scanl", "scanr", "zip", "unzip", "zipWith", "concat", "concatMap", "head", "tail",
	"init", "last", "length", "null", "reverse", "take", "drop", "splitAt", "elem",
	"notElem", "maximum", "minimum", "sum", "product", "and", "or", "any", "all",
	"iterate", "repeat", "replicate", "cycle", "takeWhile", "dropWhile", "span", "break",
	"lookup", "words", "unwords", "lines", "unlines", "show", "read", "reads", "shows",
	"error", "undefined", "seq", "flip", "const", "id", "curry", "uncurry", "until",
	"mapM", "mapM_", "forM", "forM_", "sequence", "sequence_", "forever", "void",
	"when", "unless", "liftM", "liftM2", "ap", "join", "guard", "foldM", "foldM_",
	"zipWithM", "zipWithM_", "replicateM", "replicateM_",
	NULL,
};

static const char *const kw_r[] = {
	"if", "else", "repeat", "while", "function", "for", "in", "next", "break", "TRUE",
	"FALSE", "NULL", "Inf", "NaN", "NA", "NA_integer_", "NA_real_", "NA_complex_",
	"NA_character_",
	NULL,
};

static const char *const tp_r[] = {
	"logical", "integer", "double", "complex", "character", "raw", "list", "data.frame",
	"factor", "matrix", "array", "function", "expression", "name", "call", "formula",
	NULL,
};

static const char *const bi_r[] = {
	"print", "cat", "paste", "paste0", "sprintf", "format", "nchar", "substr", "grep",
	"grepl", "sub", "gsub", "strsplit", "toupper", "tolower", "which", "match", "order",
	"rank", "sort", "rev", "unique", "duplicated", "table", "subset", "transform",
	"aggregate", "merge", "rbind", "cbind", "apply", "lapply", "sapply", "vapply",
	"mapply", "tapply", "split", "cut", "quantile", "seq", "rep", "length", "dim",
	"nrow", "ncol", "names", "colnames", "rownames", "head", "tail", "summary", "str",
	"class", "typeof", "is.na", "is.null", "is.vector", "is.list", "is.data.frame",
	"as.numeric", "as.integer", "as.character", "as.logical", "as.vector", "as.list",
	"as.data.frame", "library", "require", "source", "read.csv", "read.table", "write.csv",
	"write.table", "load", "save", "with", "within", "local", "get", "assign", "exists",
	"ls", "rm", "gc", "system", "setwd", "getwd", "dir", "list.files", "file.exists",
	"file.remove", "file.rename", "file.copy", "file.path", "basename", "dirname",
	"mean", "median", "sd", "var", "cov", "cor", "min", "max", "range", "sum", "prod",
	"cumsum", "cumprod", "cummax", "cummin", "diff", "pmax", "pmin", "rowMeans", "rowSums",
	"colMeans", "colSums", "lm", "glm", "aov", "anova", "predict", "fitted", "residuals",
	"coef", "confint", "logLik", "AIC", "BIC", "abline", "plot", "hist", "barplot",
	"boxplot", "pairs", "par",
	NULL,
};

static const char *const kw_make[] = {
	"define", "endef", "ifdef", "ifndef", "ifeq", "ifneq", "else", "endif", "include",
	"override", "export", "unexport", "private", "vpath",
	NULL,
};

static const char *const bi_make[] = {
	"MAKE", "MAKEFLAGS", "MAKEFILE", "MAKELEVEL", "MAKECMDGOALS", "CURDIR", "SHELL",
	"CC", "CXX", "CFLAGS", "CXXFLAGS", "LDFLAGS", "LDLIBS", "AR", "RM", "CP", "MV",
	"ECHO", "PWD", "TARGET_ARCH",
	NULL,
};

/* ---------------- null arrays for langs without types/builtins ---------------- */
static const char *const tp_toml[] = { NULL };
static const char *const bi_toml[] = { NULL };
static const char *const tp_yaml[] = { NULL };
static const char *const bi_yaml[] = { NULL };
static const char *const tp_json[] = { NULL };
static const char *const bi_json[] = { NULL };
static const char *const tp_bash[] = { NULL };
static const char *const tp_shell[] = { NULL };
static const char *const tp_make[] = { NULL };

/* ---------------- language table ---------------- */

static const struct lang_def lang_table[] = {
	{"c", kw_c, tp_c, bi_c, '/', '/', '*', '*', '/', '"', 0, '\'', 1, 1},
	{"cpp", kw_cpp, tp_cpp, bi_cpp, '/', '/', '*', '*', '/', '"', 0, '\'', 1, 1},
	{"java", kw_java, tp_java, bi_java, '/', '/', '*', '*', '/', '"', 0, '\'', 0, 1},
	{"python", kw_python, tp_python, bi_python, '#', 0, 0, 0, 0, '"', '\'', 0, 0, 1},
	{"rust", kw_rust, tp_rust, bi_rust, '/', '/', '*', '*', '/', '"', 0, 0, 0, 1},
	{"go", kw_go, tp_go, bi_go, '/', '/', '*', '*', '/', '"', '`', '\'', 0, 1},
	{"javascript", kw_js, tp_js, bi_js, '/', '/', '*', '*', '/', '"', '\'', 0, 0, 1},
	{"typescript", kw_ts, tp_js, bi_js, '/', '/', '*', '*', '/', '"', '\'', 0, 0, 1},
	{"ruby", kw_ruby, tp_ruby, bi_ruby, '#', 0, 0, 0, 0, '"', '\'', 0, 0, 1},
	{"php", kw_php, tp_php, bi_php, '/', '/', '*', '*', '/', '"', '\'', 0, 0, 1},
	{"swift", kw_swift, tp_swift, bi_swift, '/', '/', '*', '*', '/', '"', 0, 0, 0, 1},
	{"kotlin", kw_kotlin, tp_kotlin, bi_kotlin, '/', '/', '*', '*', '/', '"', 0, 0, 0, 1},
	{"scala", kw_scala, tp_scala, bi_scala, '/', '/', '*', '*', '/', '"', 0, '\'', 0, 1},
	{"bash", kw_bash, tp_bash, bi_bash, '#', 0, 0, 0, 0, '"', '\'', 0, 0, 1},
	{"shell", kw_shell, tp_shell, bi_shell, '#', 0, 0, 0, 0, '"', '\'', 0, 0, 1},
	{"sql", kw_sql, tp_sql, bi_sql, '-', '/', '*', '*', '/', '\'', 0, 0, 0, 0},
	{"lua", kw_lua, tp_lua, bi_lua, '-', '-', '-', ']', ']', '"', '\'', 0, 0, 1},
	{"perl", kw_perl, tp_perl, bi_perl, '#', 0, 0, 0, 0, '"', '\'', 0, 0, 1},
	{"haskell", kw_haskell, tp_haskell, bi_haskell, '-', '{', '-', '-', '}', '"', 0, 0, 0, 1},
	{"r", kw_r, tp_r, bi_r, '#', 0, 0, 0, 0, '"', '\'', 0, 0, 1},
	{"makefile", kw_make, tp_make, bi_make, '#', 0, 0, 0, 0, '"', 0, 0, 0, 1},
	{"toml", tp_toml, tp_toml, bi_toml, '#', 0, 0, 0, 0, '"', 0, 0, 0, 0},
	{"yaml", tp_yaml, tp_yaml, bi_yaml, '#', 0, 0, 0, 0, '"', '\'', 0, 0, 0},
	{"json", tp_json, tp_json, bi_json, 0, 0, 0, 0, 0, '"', 0, 0, 0, 0},
};
#define LANG_COUNT (sizeof(lang_table)/sizeof(lang_table[0]))

/* ---------------- language lookup ---------------- */

static const struct lang_def *find_lang(const char *lang, size_t len)
{
	for (size_t i = 0; i < LANG_COUNT; i++) {
		if (strlen(lang_table[i].name) == len &&
		    strncmp(lang, lang_table[i].name, len) == 0)
			return &lang_table[i];
	}
	return NULL;
}

static const struct lang_def *find_lang_alias(const char *lang, size_t len)
{
	static const struct {
		const char *alias;
		const char *real;
	} aliases[] = {
		{"js", "javascript"}, {"jsx", "javascript"},
		{"ts", "typescript"}, {"tsx", "typescript"},
		{"py", "python"},
		{"rb", "ruby"},
		{"rs", "rust"},
		{"sh", "shell"}, {"zsh", "shell"},
		{"c++", "cpp"}, {"cc", "cpp"}, {"cxx", "cpp"},
		{"h", "c"}, {"hpp", "cpp"},
		{"mk", "makefile"}, {"make", "makefile"},
		{"yml", "yaml"},
		{"hs", "haskell"},
		{"pl", "perl"},
		{"kt", "kotlin"},
		{"scala", "scala"},
		{"swift", "swift"},
	};
	for (size_t i = 0; i < sizeof(aliases)/sizeof(aliases[0]); i++) {
		if (strlen(aliases[i].alias) == len &&
		    strncmp(lang, aliases[i].alias, len) == 0) {
			const char *r = aliases[i].real;
			return find_lang(r, strlen(r));
		}
	}
	return NULL;
}

/* ---------------- keyword lookup ---------------- */

static int kw_find(const char *word, size_t wlen,
		      const char *const *table)
{
	if (!table)
		return 0;
	for (size_t i = 0; table[i]; i++) {
		size_t tlen = strlen(table[i]);
		if (tlen == wlen && memcmp(word, table[i], wlen) == 0)
			return 1;
	}
	return 0;
}

static enum hl_tok classify_ident(const char *s, size_t len,
				  const struct lang_def *lang,
				  int followed_by_paren)
{
	if (followed_by_paren && lang->has_func_call)
		return HL_TOK_FUNC;
	if (kw_find(s, len, lang->keywords))
		return HL_TOK_KEYWORD;
	if (kw_find(s, len, lang->types))
		return HL_TOK_TYPE;
	if (kw_find(s, len, lang->builtins))
		return HL_TOK_BUILTIN;
	return HL_TOK_DEFAULT;
}

/* ---------------- color for token type ---------------- */

static const char *tok_color(enum hl_tok t)
{
	switch (t) {
	case HL_TOK_KEYWORD:    return HL_KW;
	case HL_TOK_TYPE:       return HL_TYPE;
	case HL_TOK_BUILTIN:    return HL_BUILTIN;
	case HL_TOK_STRING:     return HL_STR;
	case HL_TOK_CHAR:       return HL_CHR;
	case HL_TOK_COMMENT:    return HL_CMT;
	case HL_TOK_DOC_COMMENT: return HL_DOC;
	case HL_TOK_NUMBER:     return HL_NUM;
	case HL_TOK_PREPROC:    return HL_PREP;
	case HL_TOK_FUNC:       return HL_FUNC;
	default:                return HL_DEFAULT;
	}
}

enum hl_state {
	HL_ST_DEFAULT = 0,
	HL_ST_STRING,
	HL_ST_STRING2,
	HL_ST_CHAR,
	HL_ST_COMMENT_LINE,
	HL_ST_COMMENT_BLOCK,
	HL_ST_PREPROC,
};

struct hl_scanner {
	const struct lang_def *language;
	const char *code;
	size_t code_len;
	size_t offset;
	struct hl_out output;
	enum hl_state state;
	enum hl_tok active_token;
	char string_delimiter;
	int escape;
	morph_md_highlight_newline_fn newline;
	void *newline_data;
};

static void scanner_color(struct hl_scanner *scanner, enum hl_tok token)
{
	hl_out_s(&scanner->output, tok_color(token));
}

static void scanner_newline(struct hl_scanner *scanner,
			    enum hl_tok continuation)
{
	hl_out_s(&scanner->output, HL_RESET "\n");
	if (scanner->newline)
		scanner->newline(scanner->newline_data);
	scanner->offset++;
	if (continuation != HL_TOK_DEFAULT)
		scanner_color(scanner, continuation);
}

static int is_identifier_start(char value)
{
	return (value >= 'a' && value <= 'z') ||
	       (value >= 'A' && value <= 'Z') ||
	       value == '_' || value == '$';
}

static int is_identifier_part(char value)
{
	return is_identifier_start(value) ||
	       (value >= '0' && value <= '9') ||
	       value == '!' || value == '?';
}

static int is_number_part(char value)
{
	return (value >= '0' && value <= '9') || value == '.' ||
	       value == 'e' || value == 'E' || value == 'x' ||
	       value == 'a' || value == 'b' || value == 'c' ||
	       value == 'd' || value == 'f' || value == 'A' ||
	       value == 'B' || value == 'C' || value == 'D' ||
	       value == 'F' || value == 'l' || value == 'L' ||
	       value == 'u' || value == 'U' || value == 'i' ||
	       value == 'I' || value == 'n' || value == 'N' ||
	       value == '_' || value == 'p' || value == 'P';
}

static int line_comment_at(const struct hl_scanner *scanner)
{
	char marker = scanner->language->comment_line;

	if (marker == 0 || scanner->code[scanner->offset] != marker)
		return 0;
	if (marker == '#' || marker == ';')
		return 1;
	return scanner->offset + 1u < scanner->code_len &&
	       scanner->code[scanner->offset + 1u] == marker;
}

static int block_comment_at(const struct hl_scanner *scanner)
{
	const struct lang_def *language = scanner->language;
	size_t offset = scanner->offset;

	return language->comment_multi_s != 0 &&
	       scanner->code[offset] == language->comment_multi_s &&
	       offset + 1u < scanner->code_len &&
	       scanner->code[offset + 1u] == language->comment_multi_m;
}

static void start_block_comment(struct hl_scanner *scanner)
{
	size_t offset = scanner->offset;
	enum hl_tok token = HL_TOK_COMMENT;

	if (offset + 2u < scanner->code_len &&
	    scanner->code[offset + 2u] ==
		    scanner->language->comment_multi_m)
		token = HL_TOK_DOC_COMMENT;
	scanner->active_token = token;
	scanner_color(scanner, token);
	hl_out_n(&scanner->output, scanner->code + offset, 2u);
	scanner->state = HL_ST_COMMENT_BLOCK;
	scanner->offset += 2u;
}

static void start_line_comment(struct hl_scanner *scanner)
{
	char marker = scanner->language->comment_line;
	size_t marker_len = marker == '#' || marker == ';' ? 1u : 2u;

	scanner->active_token = HL_TOK_COMMENT;
	scanner_color(scanner, HL_TOK_COMMENT);
	hl_out_n(&scanner->output, scanner->code + scanner->offset,
		 marker_len);
	scanner->state = HL_ST_COMMENT_LINE;
	scanner->offset += marker_len;
}

static void scan_number(struct hl_scanner *scanner)
{
	scanner_color(scanner, HL_TOK_NUMBER);
	while (scanner->offset < scanner->code_len &&
	       is_number_part(scanner->code[scanner->offset])) {
		hl_out_ch(&scanner->output, scanner->code[scanner->offset]);
		scanner->offset++;
	}
	scanner_color(scanner, HL_TOK_DEFAULT);
}

static void scan_identifier(struct hl_scanner *scanner)
{
	size_t start = scanner->offset;
	int followed_by_parenthesis;
	enum hl_tok token;

	while (scanner->offset < scanner->code_len &&
	       is_identifier_part(scanner->code[scanner->offset]))
		scanner->offset++;
	followed_by_parenthesis =
		scanner->offset < scanner->code_len &&
		scanner->code[scanner->offset] == '(';
	token = classify_ident(scanner->code + start,
			       scanner->offset - start,
			       scanner->language,
			       followed_by_parenthesis);
	scanner_color(scanner, token);
	hl_out_n(&scanner->output, scanner->code + start,
		 scanner->offset - start);
	scanner_color(scanner, HL_TOK_DEFAULT);
}

static void scan_default(struct hl_scanner *scanner)
{
	const struct lang_def *language = scanner->language;
	char value = scanner->code[scanner->offset];

	if (value == '#' && language->has_preproc) {
		scanner_color(scanner, HL_TOK_PREPROC);
		hl_out_ch(&scanner->output, value);
		scanner->state = HL_ST_PREPROC;
		scanner->offset++;
		return;
	}
	if (block_comment_at(scanner)) {
		start_block_comment(scanner);
		return;
	}
	if (line_comment_at(scanner)) {
		start_line_comment(scanner);
		return;
	}
	if (value == language->string_char && value != 0) {
		scanner_color(scanner, HL_TOK_STRING);
		hl_out_ch(&scanner->output, value);
		scanner->string_delimiter = value;
		scanner->state = HL_ST_STRING;
		scanner->offset++;
		return;
	}
	if (value == language->string_char2 && value != 0) {
		scanner_color(scanner, HL_TOK_STRING);
		hl_out_ch(&scanner->output, value);
		scanner->string_delimiter = value;
		scanner->state = HL_ST_STRING2;
		scanner->offset++;
		return;
	}
	if (value == language->char_char && value != 0) {
		scanner_color(scanner, HL_TOK_CHAR);
		hl_out_ch(&scanner->output, value);
		scanner->state = HL_ST_CHAR;
		scanner->offset++;
		return;
	}
	if ((value >= '0' && value <= '9') ||
	    (value == '.' && scanner->offset + 1u < scanner->code_len &&
	     scanner->code[scanner->offset + 1u] >= '0' &&
	     scanner->code[scanner->offset + 1u] <= '9')) {
		scan_number(scanner);
		return;
	}
	if (is_identifier_start(value)) {
		scan_identifier(scanner);
		return;
	}
	if (value == '\n') {
		scanner_newline(scanner, HL_TOK_DEFAULT);
		return;
	}
	hl_out_ch(&scanner->output, value);
	scanner->offset++;
}

static void scan_string(struct hl_scanner *scanner)
{
	char value = scanner->code[scanner->offset];

	if (scanner->escape) {
		scanner->escape = 0;
		hl_out_ch(&scanner->output, value);
		scanner->offset++;
		return;
	}
	if (value == '\\') {
		scanner->escape = 1;
		hl_out_ch(&scanner->output, value);
		scanner->offset++;
		return;
	}
	if (value == scanner->string_delimiter) {
		hl_out_ch(&scanner->output, value);
		scanner->state = HL_ST_DEFAULT;
		scanner_color(scanner, HL_TOK_DEFAULT);
		scanner->offset++;
		return;
	}
	if (value == '\n') {
		scanner->state = HL_ST_DEFAULT;
		scanner_newline(scanner, HL_TOK_DEFAULT);
		return;
	}
	hl_out_ch(&scanner->output, value);
	scanner->offset++;
}

static void scan_char(struct hl_scanner *scanner)
{
	char value = scanner->code[scanner->offset];

	if (scanner->escape) {
		scanner->escape = 0;
		hl_out_ch(&scanner->output, value);
		scanner->offset++;
		return;
	}
	if (value == '\\') {
		scanner->escape = 1;
		hl_out_ch(&scanner->output, value);
		scanner->offset++;
		return;
	}
	if (value == scanner->language->char_char || value == '\'') {
		hl_out_ch(&scanner->output, value);
		scanner->state = HL_ST_DEFAULT;
		scanner_color(scanner, HL_TOK_DEFAULT);
		scanner->offset++;
		return;
	}
	hl_out_ch(&scanner->output, value);
	scanner->offset++;
}

static void scan_line_comment(struct hl_scanner *scanner)
{
	char value = scanner->code[scanner->offset];

	if (value == '\n') {
		scanner->state = HL_ST_DEFAULT;
		scanner_newline(scanner, HL_TOK_DEFAULT);
		return;
	}
	hl_out_ch(&scanner->output, value);
	scanner->offset++;
}

static void scan_block_comment(struct hl_scanner *scanner)
{
	const struct lang_def *language = scanner->language;
	char value = scanner->code[scanner->offset];

	if (value == language->comment_multi_e1 &&
	    scanner->offset + 1u < scanner->code_len &&
	    scanner->code[scanner->offset + 1u] ==
		    language->comment_multi_e2) {
		hl_out_n(&scanner->output,
			 scanner->code + scanner->offset, 2u);
		scanner->state = HL_ST_DEFAULT;
		scanner_color(scanner, HL_TOK_DEFAULT);
		scanner->offset += 2u;
		return;
	}
	if (value == '\n') {
		scanner_newline(scanner, scanner->active_token);
		return;
	}
	hl_out_ch(&scanner->output, value);
	scanner->offset++;
}

static void scan_preprocessor(struct hl_scanner *scanner)
{
	char value = scanner->code[scanner->offset];

	if (value != '\n') {
		hl_out_ch(&scanner->output, value);
		scanner->offset++;
		return;
	}
	if (scanner->offset > 0u &&
	    scanner->code[scanner->offset - 1u] == '\\') {
		scanner_newline(scanner, HL_TOK_PREPROC);
		return;
	}
	scanner->state = HL_ST_DEFAULT;
	scanner_newline(scanner, HL_TOK_DEFAULT);
}

static void scan_next(struct hl_scanner *scanner)
{
	switch (scanner->state) {
	case HL_ST_DEFAULT:
		scan_default(scanner);
		break;
	case HL_ST_STRING:
	case HL_ST_STRING2:
		scan_string(scanner);
		break;
	case HL_ST_CHAR:
		scan_char(scanner);
		break;
	case HL_ST_COMMENT_LINE:
		scan_line_comment(scanner);
		break;
	case HL_ST_COMMENT_BLOCK:
		scan_block_comment(scanner);
		break;
	case HL_ST_PREPROC:
		scan_preprocessor(scanner);
		break;
	}
}

void morph_md_highlight_code(const char *lang, size_t lang_len,
			     const char *code, size_t code_len,
			     struct morph_md_sbuf *out,
			     morph_md_highlight_newline_fn newline_cb,
			     void *newline_ud)
{
	const struct lang_def *language;
	struct hl_scanner scanner;

	language = find_lang(lang, lang_len);
	if (!language)
		language = find_lang_alias(lang, lang_len);
	if (!language) {
		morph_md_sbuf_append_n(out, code, code_len);
		return;
	}

	memset(&scanner, 0, sizeof(scanner));
	scanner.language = language;
	scanner.code = code;
	scanner.code_len = code_len;
	scanner.state = HL_ST_DEFAULT;
	scanner.newline = newline_cb;
	scanner.newline_data = newline_ud;
	hl_out_init(&scanner.output, out);
	while (scanner.offset < scanner.code_len)
		scan_next(&scanner);
}

#ifndef _UTILS_JSON_H
#define _UTILS_JSON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define json_containerOf( ptr, type, member ) \
    ((type*)( (char*)ptr - offsetof( type, member ) ))

/** @defgroup tinyJson Tiny JSON parser.
  * @{ */

/** Enumeration of codes of supported JSON properties types. */
typedef enum {
    JSON_OBJ, JSON_ARRAY, JSON_TEXT, JSON_BOOLEAN,
    JSON_INTEGER, JSON_REAL, JSON_NULL
} jsonType_t;

/** Structure to handle JSON properties. */
typedef struct json_s {
    struct json_s* sibling;
    char const* name;
    union {
        char const* value;
        struct {
            struct json_s* child;
            struct json_s* last_child;
        } c;
    } u;
    jsonType_t type;
} json_t;

typedef union json_value_s {
    bool boolean;
    int64_t integer;
    double real;
    char const* string;
} json_value_t;

/** Parse a string to get a json.
  * @param str String pointer with a JSON object. It will be modified.
  * @param mem Array of json properties to allocate.
  * @param qty Number of elements of mem.
  * @retval Null pointer if any was wrong in the parse process.
  * @retval If the parser process was successfully a valid handler of a json.
  *         This property is always unnamed and its type is JSON_OBJ. */
json_t const* json_create( char* str, json_t mem[], unsigned int qty );

/** Get the name of a json property.
  * @param json A valid handler of a json property.
  * @retval Pointer to null-terminated if property has name.
  * @retval Null pointer if the property is unnamed. */
static inline char const* json_name( json_t const* property ) {
    if (property == NULL) return NULL;
    return property->name;
}

/** Get the value of a json property.
  * The type of property cannot be JSON_OBJ or JSON_ARRAY.
  * @param property A valid handler of a json property.
  * @return Pointer to null-terminated string with the value. */
static inline char const* json_string( json_t const* property ) {
    return property->u.value;
}

/** Get the type of a json property.
  * @param property A valid handler of a json property.
  * @return The code of type.*/
static inline jsonType_t json_type( json_t const* property ) {
    return property->type;
}

/** Get the next sibling of a JSON property that is within a JSON object or array.
  * @param property A valid handler of a json property.
  * @retval The handler of the next sibling if found.
  * @retval Null pointer if the json property is the last one. */
static inline json_t const* json_sibling( json_t const* property ) {
    return property->sibling;
}

/** Search a property by its name in a JSON object.
  * @param obj A valid handler of a json object. Its type must be JSON_OBJ.
  * @param property The name of property to get.
  * @retval The handler of the json property if found.
  * @retval Null pointer if not found. */
json_t const* json_property( json_t const* obj, char const* property );


/** Search a property by its name in a JSON object and return its value.
  * @param obj A valid handler of a json object. Its type must be JSON_OBJ.
  * @param property The name of property to get.
  * @retval If found a pointer to null-terminated string with the value.
  * @retval Null pointer if not found or it is an array or an object. */
json_value_t json_value( json_t const* obj, char const* property );

/** Get the first property of a JSON object or array.
  * @param property A valid handler of a json property.
  *             Its type must be JSON_OBJ or JSON_ARRAY.
  * @retval The handler of the first property if there is.
  * @retval Null pointer if the json object has not properties. */
static inline json_t const* json_child( json_t const* property ) {
    return property->u.c.child;
}

/** Get the value of a json boolean property.
  * @param property A valid handler of a json object. Its type must be JSON_BOOLEAN.
  * @return The value stdbool. */
static inline bool json_boolean( json_t const* property ) {
  return *property->u.value == 't';
}

/** Get the value of a json integer property.
  * @param property A valid handler of a json object. Its type must be JSON_INTEGER.
  * @return The value stdint. */
static inline int64_t json_integer( json_t const* property ) {
  return strtoll( property->u.value,(char**)NULL, 10);
}

/** Get the value of a json real property.
  * @param property A valid handler of a json object. Its type must be JSON_REAL.
  * @return The value. */
static inline double json_double( json_t const* property ) {
  return strtod( property->u.value,(char**)NULL );
}

/** Structure to handle a heap of JSON properties. */
typedef struct jsonPool_s jsonPool_t;
struct jsonPool_s {
    json_t* (*init)( jsonPool_t* pool );
    json_t* (*alloc)( jsonPool_t* pool );
};

/** Parse a string to get a json.
  * @param str String pointer with a JSON object. It will be modified.
  * @param pool Custom json pool pointer.
  * @retval Null pointer if any was wrong in the parse process.
  * @retval If the parser process was successfully a valid handler of a json.
  *         This property is always unnamed and its type is JSON_OBJ. */
json_t const* json_create_with_pool( char* str, jsonPool_t* pool );

/** @ } */

#ifdef __cplusplus
}
#endif

#if defined(_UTILS_IMPL) || defined(_UTILS_JSON_IMPL)

#include <string.h>
#include <ctype.h>

/** Structure to handle a heap of JSON properties. */
typedef struct jsonStaticPool_s {
    json_t* mem;      /**< Pointer to array of json properties.      */
    unsigned int qty; /**< Length of the array of json properties.   */
    unsigned int nextFree;  /**< The index of the next free json property. */
    jsonPool_t pool;
} jsonStaticPool_t;

/* Search a property by its name in a JSON object. */
json_t const* json_property( json_t const* obj, char const* property ) {
    if ( !obj || !property || json_type( obj )==JSON_NULL ) {
      return 0;
    }
    if ( obj->name && !strcmp( obj->name, property ) ) {
      return obj;
    }
    json_t const* sibling;
    for( sibling = obj->u.c.child; sibling; sibling = sibling->sibling )
        if ( sibling->name && !strcmp( sibling->name, property ) )
            return sibling;
    return 0;
}

/* Search a property by its name in a JSON object and return its typed value */
json_value_t json_value( json_t const* obj, char const* property ) {
    json_value_t value;
    value.string = NULL;
	json_t const* field = json_property( obj, property );
	if ( !field ) return value;
    jsonType_t type = json_type( field );
    if ( JSON_ARRAY >= type ) return value;
    
    switch (type) {
    case JSON_BOOLEAN:
        value.boolean = json_boolean( field );
        return value;
    case JSON_INTEGER:
        value.integer = json_integer( field );
        return value;
    case JSON_REAL:
        value.real = json_double( field );
        return value;
    case JSON_TEXT:
        value.string = json_string( field );
        return value;
    } return value;
}

/* Internal prototypes: */
static char* goBlank( char* str );
static char* goNum( char* str );
static json_t* poolInit( jsonPool_t* pool );
static json_t* poolAlloc( jsonPool_t* pool );
static char* objValue( char* ptr, json_t* obj, jsonPool_t* pool );
static char* setToNull( char* ch );
static bool isEndOfPrimitive( char ch );

/* Parse a string to get a json. */
json_t const* json_create_with_pool( char *str, jsonPool_t *pool ) {
    char* ptr = goBlank( str );
    if ( !ptr || (*ptr != '{' && *ptr != '[') ) return 0;
    json_t* obj = pool->init( pool );
    obj->name    = 0;
    obj->sibling = 0;
    obj->u.c.child = 0;
    ptr = objValue( ptr, obj, pool );
    if ( !ptr ) return 0;
    return obj;
}

/* Parse a string to get a json. */
json_t const* json_create( char* str, json_t mem[], unsigned int qty ) {
    jsonStaticPool_t spool;
    spool.mem = mem;
    spool.qty = qty;
    spool.pool.init = poolInit;
    spool.pool.alloc = poolAlloc;
    return json_create_with_pool( str, &spool.pool );
}

/** Get a special character with its escape character. Examples:
  * 'b' -> '\\b', 'n' -> '\\n', 't' -> '\\t'
  * @param ch The escape character.
  * @retval  The character code. */
static char getEscape( char ch ) {
    static struct { char ch; char code; } const pair[] = {
        { '\"', '\"' }, { '\\', '\\' },
        { '/',  '/'  }, { 'b',  '\b' },
        { 'f',  '\f' }, { 'n',  '\n' },
        { 'r',  '\r' }, { 't',  '\t' },
    };
    unsigned int i;
    for( i = 0; i < sizeof pair / sizeof *pair; ++i )
        if ( pair[i].ch == ch )
            return pair[i].code;
    return '\0';
}

/** Parse 4 characters.
  * @param str Pointer to  first digit.
  * @retval '?' If the four characters are hexadecimal digits.
  * @retval '\0' In other cases. */
static unsigned char getCharFromUnicode( unsigned char const* str ) {
    unsigned int i;
    for( i = 0; i < 4; ++i )
        if ( !isxdigit( str[i] ) )
            return '\0';
    return '?';
}

/** Parse a string and replace the scape characters by their meaning characters.
  * This parser stops when finds the character '\"'. Then replaces '\"' by '\0'.
  * @param str Pointer to first character.
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static char* parseString( char* str ) {
    unsigned char* head = (unsigned char*)str;
    unsigned char* tail = (unsigned char*)str;
    for( ; *head; ++head, ++tail ) {
        if ( *head == '\"' ) {
            *tail = '\0';
            return (char*)++head;
        }
        if ( *head == '\\' ) {
            if ( *++head == 'u' ) {
                char const ch = getCharFromUnicode( ++head );
                if ( ch == '\0' ) return 0;
                *tail = ch;
                head += 3;
            }
            else {
                char const esc = getEscape( *head );
                if ( esc == '\0' ) return 0;
                *tail = esc;
            }
        }
        else *tail = *head;
    }
    return 0;
}

/** Parse a string to get the name of a property.
  * @param ptr Pointer to first character.
  * @param property The property to assign the name.
  * @retval Pointer to first of property value. If success.
  * @retval Null pointer if any error occur. */
static char* propertyName( char* ptr, json_t* property ) {
    property->name = ++ptr;
    ptr = parseString( ptr );
    if ( !ptr ) return 0;
    ptr = goBlank( ptr );
    if ( !ptr ) return 0;
    if ( *ptr++ != ':' ) return 0;
    return goBlank( ptr );
}

/** Parse a string to get the value of a property when its type is JSON_TEXT.
  * @param ptr Pointer to first character ('\"').
  * @param property The property to assign the name.
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static char* textValue( char* ptr, json_t* property ) {
    ++property->u.value;
    ptr = parseString( ++ptr );
    if ( !ptr ) return 0;
    property->type = JSON_TEXT;
    return ptr;
}

/** Compare two strings until get the null character in the second one.
  * @param ptr sub string
  * @param str main string
  * @retval Pointer to next character.
  * @retval Null pointer if any error occur. */
static char* checkStr( char* ptr, char const* str ) {
    while( *str )
        if ( *ptr++ != *str++ )
            return 0;
    return ptr;
}

/** Parser a string to get a primitive value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @param value String with the primitive literal.
  * @param type The code of the type. ( JSON_BOOLEAN or JSON_NULL )
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static char* primitiveValue( char* ptr, json_t* property, char const* value, jsonType_t type ) {
    ptr = checkStr( ptr, value );
    if ( !ptr || !isEndOfPrimitive( *ptr ) ) return 0;
    ptr = setToNull( ptr );
    property->type = type;
    return ptr;
}

/** Parser a string to get a true value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static char* trueValue( char* ptr, json_t* property ) {
    return primitiveValue( ptr, property, "true", JSON_BOOLEAN );
}

/** Parser a string to get a false value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static char* falseValue( char* ptr, json_t* property ) {
    return primitiveValue( ptr, property, "false", JSON_BOOLEAN );
}

/** Parser a string to get a null value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type, (true, false or null).
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static char* nullValue( char* ptr, json_t* property ) {
    return primitiveValue( ptr, property, "null", JSON_NULL );
}

/** Analyze the exponential part of a real number.
  * @param ptr Pointer to first character.
  * @retval Pointer to first non numerical after the string. If success.
  * @retval Null pointer if any error occur. */
static char* expValue( char* ptr ) {
    if ( *ptr == '-' || *ptr == '+' ) ++ptr;
    if ( !isdigit( (int)(*ptr) ) ) return 0;
    ptr = goNum( ++ptr );
    return ptr;
}

/** Analyze the decimal part of a real number.
  * @param ptr Pointer to first character.
  * @retval Pointer to first non numerical after the string. If success.
  * @retval Null pointer if any error occur. */
static char* fraqValue( char* ptr ) {
    if ( !isdigit( (int)(*ptr) ) ) return 0;
    ptr = goNum( ++ptr );
    if ( !ptr ) return 0;
    return ptr;
}

/** Parser a string to get a numerical value.
  * If the first character after the value is different of '}' or ']' is set to '\0'.
  * @param ptr Pointer to first character.
  * @param property Property handler to set the value and the type: JSON_REAL or JSON_INTEGER.
  * @retval Pointer to first non white space after the string. If success.
  * @retval Null pointer if any error occur. */
static char* numValue( char* ptr, json_t* property ) {
    if ( *ptr == '-' ) ++ptr;
    if ( !isdigit( (int)(*ptr) ) ) return 0;
    if ( *ptr != '0' ) {
        ptr = goNum( ptr );
        if ( !ptr ) return 0;
    }
    else if ( isdigit( (int)(*++ptr) ) ) return 0;
    property->type = JSON_INTEGER;
    if ( *ptr == '.' ) {
        ptr = fraqValue( ++ptr );
        if ( !ptr ) return 0;
        property->type = JSON_REAL;
    }
    if ( *ptr == 'e' || *ptr == 'E' ) {
        ptr = expValue( ++ptr );
        if ( !ptr ) return 0;
        property->type = JSON_REAL;
    }
    if ( !isEndOfPrimitive( *ptr ) ) return 0;
    if ( JSON_INTEGER == property->type ) {
        char const* value = property->u.value;
        bool const negative = *value == '-';
        static char const min[] = "-9223372036854775808";
        static char const max[] = "9223372036854775807";
        unsigned int const maxdigits = ( negative? sizeof min: sizeof max ) - 1;
        unsigned int const len = ( unsigned int const ) ( ptr - value );
        if ( len > maxdigits ) return 0;
        if ( len == maxdigits ) {
            char const tmp = *ptr;
            *ptr = '\0';
            char const* const threshold = negative ? min: max;
            if ( 0 > strcmp( threshold, value ) ) return 0;
            *ptr = tmp;
        }
    }
    ptr = setToNull( ptr );
    return ptr;
}

/** Add a property to a JSON object or array.
  * @param obj The handler of the JSON object or array.
  * @param property The handler of the property to be added. */
static void add( json_t* obj, json_t* property ) {
    property->sibling = 0;
    if ( !obj->u.c.child ){
	    obj->u.c.child = property;
	    obj->u.c.last_child = property;
    } else {
	    obj->u.c.last_child->sibling = property;
	    obj->u.c.last_child = property;
    }
}

/** Parser a string to get a json object value.
  * @param ptr Pointer to first character.
  * @param obj The handler of the JSON root object or array.
  * @param pool The handler of a json pool for creating json instances.
  * @retval Pointer to first character after the value. If success.
  * @retval Null pointer if any error occur. */
static char* objValue( char* ptr, json_t* obj, jsonPool_t* pool ) {
    obj->type    = *ptr == '{' ? JSON_OBJ : JSON_ARRAY;
    obj->u.c.child = 0;
    obj->sibling = 0;
    ptr++;
    for(;;) {
        ptr = goBlank( ptr );
        if ( !ptr ) return 0;
        if ( *ptr == ',' ) {
            ++ptr;
            continue;
        }
        char const endchar = ( obj->type == JSON_OBJ )? '}': ']';
        if ( *ptr == endchar ) {
            *ptr = '\0';
            json_t* parentObj = obj->sibling;
            if ( !parentObj ) return ++ptr;
            obj->sibling = 0;
            obj = parentObj;
            ++ptr;
            continue;
        }
        json_t* property = pool->alloc( pool );
        if ( !property ) return 0;
        if( obj->type != JSON_ARRAY ) {
            if ( *ptr != '\"' ) return 0;
            ptr = propertyName( ptr, property );
            if ( !ptr ) return 0;
        }
        else property->name = 0;
        add( obj, property );
        property->u.value = ptr;
        switch( *ptr ) {
            case '{':
                property->type    = JSON_OBJ;
                property->u.c.child = 0;
                property->sibling = obj;
                obj = property;
                ++ptr;
                break;
            case '[':
                property->type    = JSON_ARRAY;
                property->u.c.child = 0;
                property->sibling = obj;
                obj = property;
                ++ptr;
                break;
            case '\"': ptr = textValue( ptr, property );  break;
            case 't':  ptr = trueValue( ptr, property );  break;
            case 'f':  ptr = falseValue( ptr, property ); break;
            case 'n':  ptr = nullValue( ptr, property );  break;
            default:   ptr = numValue( ptr, property );   break;
        }
        if ( !ptr ) return 0;
    }
}

/** Initialize a json pool.
  * @param pool The handler of the pool.
  * @return a instance of a json. */
static json_t* poolInit( jsonPool_t* pool ) {
    jsonStaticPool_t *spool = json_containerOf( pool, jsonStaticPool_t, pool );
    spool->nextFree = 1;
    return spool->mem;
}

/** Create an instance of a json from a pool.
  * @param pool The handler of the pool.
  * @retval The handler of the new instance if success.
  * @retval Null pointer if the pool was empty. */
static json_t* poolAlloc( jsonPool_t* pool ) {
    jsonStaticPool_t *spool = json_containerOf( pool, jsonStaticPool_t, pool );
    if ( spool->nextFree >= spool->qty ) return 0;
    return spool->mem + spool->nextFree++;
}

/** Checks whether an character belongs to set.
  * @param ch Character value to be checked.
  * @param set Set of characters. It is just a null-terminated string.
  * @return true or false there is membership or not. */
static bool isOneOfThem( char ch, char const* set ) {
    while( *set != '\0' )
        if ( ch == *set++ )
            return true;
    return false;
}

/** Increases a pointer while it points to a character that belongs to a set.
  * @param str The initial pointer value.
  * @param set Set of characters. It is just a null-terminated string.
  * @return The final pointer value or null pointer if the null character was found. */
static char* goWhile( char* str, char const* set ) {
    for(; *str != '\0'; ++str ) {
        if ( !isOneOfThem( *str, set ) )
            return str;
    }
    return 0;
}

/** Set of characters that defines a blank. */
static char const* const blank = " \n\r\t\f";

/** Increases a pointer while it points to a white space character.
  * @param str The initial pointer value.
  * @return The final pointer value or null pointer if the null character was found. */
static char* goBlank( char* str ) {
    return goWhile( str, blank );
}

/** Increases a pointer while it points to a decimal digit character.
  * @param str The initial pointer value.
  * @return The final pointer value or null pointer if the null character was found. */
static char* goNum( char* str ) {
    for( ; *str != '\0'; ++str ) {
        if ( !isdigit( (int)(*str) ) )
            return str;
    }
    return 0;
}

/** Set of characters that defines the end of an array or a JSON object. */
static char const* const endofblock = "}]";

/** Set a char to '\0' and increase its pointer if the char is different to '}' or ']'.
  * @param ch Pointer to character.
  * @return  Final value pointer. */
static char* setToNull( char* ch ) {
    if ( !isOneOfThem( *ch, endofblock ) ) *ch++ = '\0';
    return ch;
}

/** Indicate if a character is the end of a primitive value. */
static bool isEndOfPrimitive( char ch ) {
    return ch == ',' || isOneOfThem( ch, blank ) || isOneOfThem( ch, endofblock );
}

#endif
#endif
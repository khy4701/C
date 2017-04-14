/** 
 * @file json.c
 * @brief JSON Parser
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SCS_TIME_LEN            19
#define MAX_SCS_COMM_IND_LEN        32
#define MAX_SCS_STATIONARY_LEN      10  // Stationary, Mobile

#define MAX_JSON_TOKEN_SIZE			100
#define HTTPF_MSG_BUFSIZE			10240
#define MAX_SCS_EVT_TYPE_NUM        3   // sms, data, both
#define MAX_SCS_EVT_TYPE_LEN        4
#define MAX_SCS_LOC_TYPE_LEN        7   // current, last
#define MAX_SCS_ACCURACY_LEN        4   // ecgi, enb, ta, pra
#define MAX_SCS_GRP_TYPE_LEN        5   // tai, enbid
#define MAX_SCS_GRP_VAL_LEN         64
#define MAX_SCS_MON_TYPE_LEN        32
#define MAX_SCS_CHARGE_NUM_LEN      32
#define MAX_SCS_ASS_TYPE_NUM        2   // IMSI, IMEISV
#define MAX_SCS_ASSO_MON_LEN		6	// IMSI, IMEISV
#define MAX_SCS_MON_SET_NUM         10
#define MAX_SCS_ID_LEN				32
#define MAX_SCS_REF_ID_LEN			32



typedef struct  {
    char    start_time[MAX_SCS_TIME_LEN+1];
    char    expiry_time[MAX_SCS_TIME_LEN+1];
} Scs_validity_t;

typedef struct  {
    int     event_number;
    char    event_type[MAX_SCS_EVT_TYPE_NUM][MAX_SCS_EVT_TYPE_LEN+1];
    int     max_latency;
    int     max_resp;
} Scs_rchble_mon_t;

typedef struct  {
    char    loc_type[MAX_SCS_LOC_TYPE_LEN+1];
    char    accuracy[MAX_SCS_ACCURACY_LEN+1];
} Scs_loc_mon_t;

typedef struct  {
    char    group_type[MAX_SCS_GRP_TYPE_LEN+1];
    char    group_value[MAX_SCS_GRP_VAL_LEN+1];
} Scs_group_mon_t;

typedef struct  {
    int                 scef_ref_id;
    char                mon_type[MAX_SCS_MON_TYPE_LEN+1];
    Scs_validity_t      validity;
    int                 report_num;
    char                charging_num[MAX_SCS_CHARGE_NUM_LEN+1];
    Scs_rchble_mon_t    rch_mon;
    char                ass_mon[MAX_SCS_ASS_TYPE_NUM][MAX_SCS_ASSO_MON_LEN+1];
    Scs_loc_mon_t       loc_mon;
    Scs_group_mon_t     grp_mon;
} Scs_mon_set_t;


/**
 * JSON type identifier. Basic types are:
 *  o Object
 *  o Array
 *  o String
 *  o Other primitive: number, boolean (true/false) or null
 */
typedef enum { JSON_PRIMITIVE, JSON_OBJECT, JSON_ARRAY, JSON_STRING } JsonType;

typedef enum
{
    JSON_ERROR_NOMEM = -1, // Not enough tokens were provided
    JSON_ERROR_INVAL = -2, // Invalid character inside JSON string
    JSON_ERROR_PART = -3, // The string is not a full JSON packet, more bytes expected
    JSON_SUCCESS = 0 // Everthing is fine
} JsonError;

/**
 * JSON token description.
 * @param       type    type (object, array, string etc.)
 * @param       start   start position in JSON data string
 * @param       end     end position in JSON data string
 */
typedef struct
{
    JsonType type;
    int start;
    int end;
    int size;
    #ifdef json_PARENT_LINKS
    int parent;
    #endif
} JsonToken;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct
{
    unsigned int pos; /* offset in the JSON string */
    unsigned int toknext; /* next token to allocate */
    int toksuper; /* superior token node, e.g parent object or array */
} JsonParser;

/**
 * Create JSON parser over an array of tokens
 */
void json_initJsonParser(JsonParser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
JsonError json_parseJson(JsonParser *parser, const char *js, JsonToken *tokens, unsigned int tokenNum);


/**
 * @fn static JsonToken *json_allocJsonToken(JsonParser *parser, JsonToken *tokens, size_t tokenNum)
 * @brief Allocates a fresh unused token from the token pull.
 * @param parser
 * @param tokens
 * @param tokenNum
 */
static JsonToken *json_allocJsonToken(JsonParser *parser, JsonToken *tokens, size_t tokenNum)
{
    if (parser->toknext >= tokenNum) return NULL;
    JsonToken *tok = &tokens[parser->toknext++];
    tok->start = tok->end = -1;
    tok->size = 0;
    #ifdef json_PARENT_LINKS
    tok->parent = -1;
    #endif
    return tok;
}

/**
 * @fn static void json_fillToken(JsonToken *token, JsonType type, int start, int end)
 * @brief Fills token type and boundaries.
 * @param token
 * @param type
 * @param start
 * @param end
 */
static void json_fillToken(JsonToken *token, JsonType type, int start, int end)
{
    token->type = type;
    token->start = start;
    token->end = end;
    token->size = 0;
}

/**
 * @fn static JsonError json_parsePrimitive(JsonParser *parser, const char *js, JsonToken *tokens, size_t num_tokens)
 * @brief Fills next available token with JSON primitive.
 * @param parser
 * @param js
 * @param tokens
 * @param num_tokens
 */
static JsonError json_parsePrimitive(JsonParser *parser, const char *js, JsonToken *tokens, size_t num_tokens)
{
    JsonToken *token;
    int start;

    start = parser->pos;

    for (; js[parser->pos] != '\0'; parser->pos++)
    {
        switch (js[parser->pos]) 
        {
            #ifndef json_STRICT
            /* In strict mode primitive must be followed by "," or "}" or "]" */
            case ':':
            #endif
            case '\t': 
            case '\r': 
            case '\n': 
            case ' ':
            case ',': 
            case ']': 
            case '}':
                goto found;
        }
        if (js[parser->pos] < 32 || js[parser->pos] >= 127)
        {
            parser->pos = start;
            return JSON_ERROR_INVAL;
        }
    }
    #ifdef json_STRICT
    /* In strict mode primitive must be followed by a comma/object/array */
    parser->pos = start;
    return JSON_ERROR_PART;
    #endif

    found:
    token = json_allocJsonToken(parser, tokens, num_tokens);
    if (!token)
    {
        parser->pos = start;
        return JSON_ERROR_NOMEM;
    }
    json_fillToken(token, JSON_PRIMITIVE, start, parser->pos);
    #ifdef json_PARENT_LINKS
    token->parent = parser->toksuper;
    #endif
    parser->pos--;
    return JSON_SUCCESS;
}

/**
 * @fn static JsonError json_parseString(JsonParser *parser, const char *js, JsonToken *tokens, size_t num_tokens)
 * @brief Fills next token with JSON string.
 * @param parser
 * @param js 
 * @param tokens
 * @param num_tokens
 */
static JsonError json_parseString(JsonParser *parser, const char *js, JsonToken *tokens, size_t num_tokens)
{
    JsonToken *token;
    int start = parser->pos;

    parser->pos++;

    /* Skip starting quote */
    for (; js[parser->pos] != '\0'; parser->pos++)
    {
        char c = js[parser->pos];

        /* Quote: end of string */
        if (c == '\"')
        {
            token = json_allocJsonToken(parser, tokens, num_tokens);
            if (!token)
            {
                parser->pos = start;
                return JSON_ERROR_NOMEM;
            }
            json_fillToken(token, JSON_STRING, start+1, parser->pos);
            #ifdef json_PARENT_LINKS
            token->parent = parser->toksuper;
            #endif
            return JSON_SUCCESS;
        }

        /* Backslash: Quoted symbol expected */
        if (c == '\\')
        {
            parser->pos++;
            switch (js[parser->pos])
            {
                /* Allowed escaped symbols */
                case '\"': 
                case '/': 
                case '\\': 
                case 'b':
                case 'f': 
                case 'r': 
                case 'n': 
                case 't':
                    break;
                /* Allows escaped symbol \uXXXX */
                case 'u':
                    /// \todo handle JSON unescaped symbol \\uXXXX
                    break;
                /* Unexpected symbol */
                default:
                    parser->pos = start;
                    return JSON_ERROR_INVAL;
            }
        }
    }
    parser->pos = start;
    return JSON_ERROR_PART;
}

/**
 * @fn JsonError json_parseJson(JsonParser *parser, const char *js, JsonToken *tokens, unsigned int num_tokens) 
 * @brief Parse JSON string and fill tokens.
 * @param parser
 * @param js
 * @param tokens
 * @param num_tokens
 */
JsonError json_parseJson(JsonParser *parser, const char *js, JsonToken *tokens, unsigned int num_tokens) 
{
    JsonError r;
    int i;
    JsonToken *token;

    for (; js[parser->pos] != '\0'; parser->pos++)
    {
        char c;
        JsonType type;

        c = js[parser->pos];
        switch (c)
        {
            case '{':
            case '[':
                token = json_allocJsonToken(parser, tokens, num_tokens);
                if (!token) return JSON_ERROR_NOMEM;
                if (parser->toksuper != -1)
                {
                    tokens[parser->toksuper].size++;
                    #ifdef json_PARENT_LINKS
                    token->parent = parser->toksuper;
                    #endif
                }
                token->type = (c == '{' ? JSON_OBJECT : JSON_ARRAY);
                token->start = parser->pos;
                parser->toksuper = parser->toknext - 1;
                break;
            case '}':
            case ']':
                type = (c == '}' ? JSON_OBJECT : JSON_ARRAY);
                #ifdef json_PARENT_LINKS
                if (parser->toknext < 1) {
					puts("### 1 ###");
					return JSON_ERROR_INVAL;
				}
                token = &tokens[parser->toknext - 1];
                for (;;)
                {
                    if (token->start != -1 && token->end == -1)
                    {
                        if (token->type != type) {
							puts("### 2 ###");
							return JSON_ERROR_INVAL;
						}
                        token->end = parser->pos + 1;
                        parser->toksuper = token->parent;
                        break;
                    }
                    if (token->parent == -1) break;
                    token = &tokens[token->parent];
                }
                #else
                for (i = parser->toknext - 1; i >= 0; i--)
                {
                    token = &tokens[i];
                    if (token->start != -1 && token->end == -1)
                    {
                        if (token->type != type) {
							puts("### 3 ###");
							return JSON_ERROR_INVAL;
						}
                        parser->toksuper = -1;
                        token->end = parser->pos + 1;
                        break;
                    }
                }
                /* Error if unmatched closing bracket */
                if (i == -1) {
					puts("### 4 ###");
					return JSON_ERROR_INVAL;
				}
                for (; i >= 0; i--)
                {
                    token = &tokens[i];
                    if (token->start != -1 && token->end == -1)
                    {
                        parser->toksuper = i;
                        break;
                    }
                }
                #endif
                break;
            case '\"':
                r = json_parseString(parser, js, tokens, num_tokens);
                if (r < 0) return r;
                if (parser->toksuper != -1) tokens[parser->toksuper].size++;
                break;
            case '\t':
            case '\r':
            case '\n':
            case ':':
            case ',':
            case ' ': 
                break;
            #ifdef json_STRICT
            /* In strict mode primitives are: numbers and booleans */
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case 't':
            case 'f':
            case 'n':
            #else
            /* In non-strict mode every unquoted value is a primitive */
            default:
            #endif
                r = json_parsePrimitive(parser, js, tokens, num_tokens);
                if (r < 0) return r;
                if (parser->toksuper != -1) tokens[parser->toksuper].size++;
                break;

            #ifdef json_STRICT
            /* Unexpected char in strict mode */
            default:
				puts("### 5 ###");
                return JSON_ERROR_INVAL;
            #endif

        }
    }

    for (i = parser->toknext - 1; i >= 0; i--)
    {
        /* Unmatched opened object or array */
        if (tokens[i].start != -1 && tokens[i].end == -1) return JSON_ERROR_PART;
    }

    return JSON_SUCCESS;
}

/**
 * @fn void json_initJsonParser(JsonParser *parser)
 * @brief Creates a new parser based over a given buffer with an array of tokens available.
 * @param parser
 */
void json_initJsonParser(JsonParser *parser)
{
    parser->pos = 0;
    parser->toknext = 0;
    parser->toksuper = -1;
}

int json_object_index_get(char *json_str, JsonToken *tokens, char *str)
{
    int i;
    char    tmpStr[HTTPF_MSG_BUFSIZE];

    for (i=0; i < MAX_JSON_TOKEN_SIZE; i++) {
        if (tokens[i].start == 0 && tokens[i].end == 0) break;
        strncpy(tmpStr, &json_str[tokens[i].start], tokens[i].end - tokens[i].start);
        tmpStr[tokens[i].end - tokens[i].start] = 0;
        if (!strcmp(tmpStr, str)) return i;
    }

    return -1;
}

int get_int_from_json_new(char *json_str, JsonToken *tokens, char *key, int *result)
{
    int     i, length;
    char    str_ptr[1024];

    if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }

    i++;
    length = tokens[i].end - tokens[i].start;
    strncpy(str_ptr, &json_str[tokens[i].start], length);
    str_ptr[length] = 0;

    *result = atoi(str_ptr);

    return 1;
}

int get_string_from_json_new(char *json_str, JsonToken *tokens, char *key, char *result, int max_len)
{
    int     i, length;

    if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
	}

	i++;
	length = tokens[i].end - tokens[i].start;
	if (length > max_len) {
        printf("[%s] can't get(%s) length(%d) is too big\n", __func__, key, length);
        return -1;
	}

	strncpy(result, &json_str[tokens[i].start], length);
	result[length] = 0;

    return 1;
}

int get_address_array_from_json_new(char *json_str, JsonToken *tokens, char *key, char array_str[10][32+1], int max_item, int max_len)
{
	int    num, array_num, i, j, length;
    struct json_object *aso, *rso;
    const char *str_ptr;
	char	arr_str[1024];

    if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }
	i++;
    if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
        printf("[%s] can't get key(%s) array number(%d) from json token size\n", __func__, key, i);
        return -1;
	}
	array_num = tokens[i].size;
    if (array_num > max_item || array_num <= 0) {
        printf("[%s] key(%s) array number(%d) is over %d\n", __func__, key, array_num, max_item);
        return -1;
    }

    for (j=0; j < array_num; j++) {
		i++;
        if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
        	printf("[%s] can't get %s[%d] is too big in json token\n", __func__, key, j);
        	return -1;
		}
		length = tokens[i].end - tokens[i].start;
		if (length > max_len) {
        	printf("[%s] can't get(%s) length(%d) is too big\n", __func__, key, length);
        	return -1;
		}

		strncpy(array_str[j], &json_str[tokens[i].start], length);
		array_str[j][length] = 0;
    }
    return array_num;
}


int get_evttype_array_from_json_new(char *json_str, JsonToken *tokens, char *key, char array_str[MAX_SCS_EVT_TYPE_NUM][MAX_SCS_EVT_TYPE_LEN+1], int max_item, int max_len)
{
    int     num, array_num, i, j, length;
    struct json_object *aso, *rso;
    const char *str_ptr;

	if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }
    i++;
    if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
        printf("[%s] can't get key(%s) array number(%d) from json token\n", __func__, key, i);
        return -1;
    }
    array_num = tokens[i].size;
    if (array_num > max_item || array_num <= 0) {
        printf("[%s] key(%s) array number(%d) is over %d\n", __func__, key, array_num, max_item);
        return -1;
    }

    for (j=0; j < array_num; j++) {
		i++;
        if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
            printf("[%s] can't get %s[%d] is too big in json token\n", __func__, key, j);
            return -1;
        }
        length = tokens[i].end - tokens[i].start;
        if (length > max_len) {
            printf("[%s] can't get(%s) length is too big", __func__, key, length);
            return -1;
        }

		strncpy(array_str[j], &json_str[tokens[i].start], length);
        array_str[j][length] = 0;
    }
    return array_num;
}


int get_assomon_array_from_json_new(char *json_str, JsonToken *tokens, char *key, char array_str[MAX_SCS_ASS_TYPE_NUM][MAX_SCS_ASSO_MON_LEN+1], int max_item, int max_len)
{
    int    num, array_num, i, j, length;
    struct json_object *aso, *rso;
    const char *str_ptr;
    char    arr_str[1024];

    if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }
    i++;
    if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
        printf("[%s] can't get key(%s) array number(%d) from json token\n", __func__, key, i);
        return -1;
    }
    array_num = tokens[i].size;
    if (array_num > max_item || array_num <= 0) {
        printf("[%s] key(%s) array number(%d) is over %d\n", __func__, key, array_num, max_item);
        return -1;
    }

    for (j=0; j < array_num; j++) {
        i++;
        if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
            printf("[%s] can't get %s[%d] is too big in json token\n", __func__, key, j);
            return -1;
        }
        length = tokens[i].end - tokens[i].start;
        if (length > max_len) {
            printf("[%s] can't get(%s) length(%d) is too big\n", __func__, key, length);
            return -1;
        }

        strncpy(array_str[j], &json_str[tokens[i].start], length);
        array_str[j][length] = 0;
    }
    return array_num;
}


int get_validity_from_json_new(char *json_str, JsonToken *tokens, char *key, Scs_validity_t *validity)
{
	int    num, data_num, i, j, length, max_item = 4;
    struct json_object *aso, *rso;
    const char *str_ptr;
	char	arr_str[1024];

    if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }
	i++;
    if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
        printf("[%s] can't get key(%s) from json token\n", __func__, key);
        return -1;
	}
	data_num = tokens[i].size;
    if (data_num > max_item || data_num <= 0) {
        printf("[%s] key(%s) array number(%d) is over %d\n", __func__, key, data_num, max_item);
        return -1;
    }

	// startTime (Mandatory)
 	if ( get_string_from_json_new(json_str, &tokens[i], "startTime", validity->start_time, MAX_SCS_TIME_LEN) < 0) {
		printf("can't get(%s) data from json: i=[%d]\n", "startTime", i);
		return;
	} else {
		printf("[DATA] %s:[%s]\n", "startTime", validity->start_time);
	}

	// expiryTime (Mandatory)
    if ( get_string_from_json_new(json_str, &tokens[i], "expiryTime", validity->expiry_time, MAX_SCS_TIME_LEN) < 0) {
        printf("can't get(%s) data from json: i=[%d]\n", "expiryTime", i);
        return;
    } else {
		printf("[DATA] %s:[%s]\n", "expiryTime", validity->expiry_time);
    }

    return data_num;
}


JsonToken *json_object_object_get_new(char *json_str, JsonToken *tokens, char *key) 
{
	int    num, data_num, i, j, length, max_item = 4;
    struct json_object *aso, *rso;
    const char *str_ptr;
	char	arr_str[1024];

    if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return NULL;
    }
	i++;
	if ((i >= MAX_JSON_TOKEN_SIZE) || (tokens[i].start == 0 && tokens[i].end == 0)) {
        printf("[%s] can't get key(%s) object number(%d) from json token\n", __func__, key, i);
        return NULL;
	}

	return &tokens[i];
}


JsonToken *json_object_array_get_idx_new(char *json_str, JsonToken *tokens, int idx) 
{
	int i, array_cnt = 0, next_array_offset = -1;

    for (i=0; i < MAX_JSON_TOKEN_SIZE; i++) {
        if (tokens[i].start == 0 && tokens[i].end == 0) break;
		if (tokens[i].start <= next_array_offset) continue;
		if (tokens[i].type == JSON_OBJECT) {
			array_cnt++;
			next_array_offset = tokens[i].end;
		}
		if (array_cnt == (idx+1)) return &tokens[i];
    }

	return NULL;
}


int get_int_bit_mask_from_json_new(char *json_str, JsonToken *tokens, char *key, int *result)
{
    int     i, length, ibitmask=0;
    char    str_ptr[1024];

    if ((i = json_object_index_get(json_str, tokens, key)) < 0) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }

    i++;
    length = tokens[i].end - tokens[i].start;
    strncpy(str_ptr, &json_str[tokens[i].start], length);
    str_ptr[length] = 0;

    length = strlen(str_ptr);
    for (i=0; i<length; i++) {
        if (str_ptr[i] != '0') ibitmask = (ibitmask<<1) | 1;
        else ibitmask <<= 1;
    }

    *result = ibitmask;
    return 1;
}


#define MAX_SCS_SCHEDULE_COMM_NUM   10

typedef struct  {
    int     time_of_day_start;
    int     time_of_day_end;
    int     day_of_week_mask;
    int     timezone_flag;
} Scs_schedule_comm_t;


int get_sched_comm_array_from_json_new(char *json_str, JsonToken *tokens, char *key, Scs_schedule_comm_t sched_comm[MAX_SCS_SCHEDULE_COMM_NUM], int max_item)
{
    int     num, array_num, i, j, length, ret;
    struct json_object *aso, *rso;
    Scs_schedule_comm_t *sched_comm_ptr;
	char        schedCommStr[HTTPF_MSG_BUFSIZE], bb[1024], cc[1024];
    JsonToken   schedCommTokens[MAX_JSON_TOKEN_SIZE], *tokenPtr;
    JsonParser  schedCommParser;

    if ((tokenPtr = json_object_object_get_new(json_str, tokens, key)) == NULL) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }
	strncpy(schedCommStr, &json_str[tokenPtr->start], tokenPtr->end - tokenPtr->start);
    json_initJsonParser(&schedCommParser);
    memset(&schedCommTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
    if ((ret = json_parseJson(&schedCommParser, (const char *)schedCommStr, schedCommTokens, MAX_JSON_TOKEN_SIZE)) < 0) {
        printf("[%s] can't parse periodicCommunication(%s) ret(%d)\n", __func__, schedCommStr, ret);
        return -1;
    }

	array_num = tokenPtr->size;
    if (array_num > max_item || array_num <= 0) {
        printf("[%s] key(%s) array number(%d) is over %d\n", __func__, key, array_num, max_item);
        return -1;
    } else {
        printf("[%s] key(%s) array number(%d) and max array number(%d).\n", __func__, key, array_num, max_item);
	}

#if 0
	printf("\n\n\n###############################################\nschedCommStr:\n");

	for (i=0; i < MAX_JSON_TOKEN_SIZE; i++) {
		if (schedCommTokens[i].start == 0 && schedCommTokens[i].end == 0)
			break;
	
		strncpy(bb, &schedCommStr[schedCommTokens[i].start], schedCommTokens[i].end - schedCommTokens[i].start);
		bb[schedCommTokens[i].end - schedCommTokens[i].start] = 0;
		printf("i=%d %d %d %d %d [%s]\n", 
				i, schedCommTokens[i].type, schedCommTokens[i].start, schedCommTokens[i].end, schedCommTokens[i].size, bb);

	}
	printf("####################################################\n");
#endif

	char        schedCommArrayStr[HTTPF_MSG_BUFSIZE];
    JsonToken   schedCommArrayTokens[MAX_JSON_TOKEN_SIZE];
    JsonParser  schedCommArrayParser;
    for (i=0; i < array_num; i++) {
        if ((tokenPtr = json_object_array_get_idx_new(schedCommStr, schedCommTokens, i)) == NULL) {
        	printf("[%s] can't get schedCommArray[%d] from (%s) ret(%d)\n", __func__, i, schedCommArrayStr, ret);
        	return -1;
		}
		strncpy(schedCommArrayStr, &schedCommStr[tokenPtr->start], tokenPtr->end - tokenPtr->start);
    	json_initJsonParser(&schedCommArrayParser);
    	memset(&schedCommArrayTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
    	if ((ret = json_parseJson(&schedCommArrayParser, (const char *)schedCommArrayStr, schedCommArrayTokens, MAX_JSON_TOKEN_SIZE)) < 0) {
        	printf("[%s] can't parse schedCommArray(%s) ret(%d)\n", __func__, schedCommArrayStr, ret);
        	return -1;
    	}

#if 0
		printf("\n\n\n###############################################\nschedCommArrayStr:[%s]\n", schedCommArrayStr);

		for (j=0; j < MAX_JSON_TOKEN_SIZE; j++) {
			if (schedCommArrayTokens[j].start == 0 && schedCommArrayTokens[j].end == 0) break;
	
			strncpy(bb, &schedCommArrayStr[schedCommArrayTokens[j].start], schedCommArrayTokens[j].end - schedCommArrayTokens[j].start);
			bb[schedCommArrayTokens[j].end - schedCommArrayTokens[j].start] = 0;
			printf("j=%d %d %d %d %d [%s]\n", 
					j, schedCommArrayTokens[j].type, schedCommArrayTokens[j].start, schedCommArrayTokens[j].end, schedCommArrayTokens[j].size, bb);

		}
		printf("####################################################\n");
#endif

        sched_comm_ptr = &sched_comm[i];

        // timeOfDayStart (Optional)
        if (get_int_from_json_new(schedCommArrayStr, schedCommArrayTokens, "timeOfDayStart", &sched_comm_ptr->time_of_day_start) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "timeOfDayStart");
            sched_comm_ptr->time_of_day_start = -1;
        } else {
			printf("[DATA] timeOfDayStart[%d]=[%d]\n", i, sched_comm_ptr->time_of_day_start);
		}

        // timeOfDayEnd (Optional)
        if (get_int_from_json_new(schedCommArrayStr, schedCommArrayTokens, "timeOfDayEnd", &sched_comm_ptr->time_of_day_end) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "timeOfDayEnd");
            sched_comm_ptr->time_of_day_end = -1;
        } else {
			printf("[DATA] timeOfDayEnd[%d]=[%d]\n", i, sched_comm_ptr->time_of_day_end);
        }

        // dayOfWeekMask (Optional)
        if (get_int_bit_mask_from_json_new(schedCommArrayStr, schedCommArrayTokens, "dayOfWeekMask", &sched_comm_ptr->day_of_week_mask) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "dayOfWeekMask");
        } else {
			printf("[DATA] dayOfWeekMask[%d]=[%d]\n", i, sched_comm_ptr->day_of_week_mask);
        }

        // timezoneFlag (Optional)
        if (get_int_from_json_new(schedCommArrayStr, schedCommArrayTokens, "timezoneFlag", &sched_comm_ptr->timezone_flag) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "timezoneFlag");
        } else {
			printf("[DATA] timezoneFlag[%d]=[%d]\n", i, sched_comm_ptr->timezone_flag);
        }
    }

    return array_num;
}


int get_monset_array_from_json_new(char *json_str, JsonToken *tokens, char *key, Scs_mon_set_t mon_set[MAX_SCS_MON_SET_NUM], int max_item)
{
    int     num, array_num, evt_array_num, ass_array_num, i, j, length, ret;
    struct json_object  *aso, *rso;
    Scs_mon_set_t       *mon_set_ptr;
    Scs_validity_t      *validity_ptr;
    Scs_rchble_mon_t    *rch_mon_ptr;
    Scs_loc_mon_t       *loc_mon_ptr;
    Scs_group_mon_t     *grp_mon_ptr;
	char        monSetStr[HTTPF_MSG_BUFSIZE], bb[HTTPF_MSG_BUFSIZE], cc[HTTPF_MSG_BUFSIZE];
    JsonToken   monSetTokens[MAX_JSON_TOKEN_SIZE], *tokenPtr;
    JsonParser  monSetParser;


    if ((tokenPtr = json_object_object_get_new(json_str, tokens, key)) == NULL) {
        printf("[%s] can't get %s\n", __func__, key);
        return -1;
    }
	memset(monSetStr, 0x00, HTTPF_MSG_BUFSIZE);
    strncpy(monSetStr, &json_str[tokenPtr->start], tokenPtr->end - tokenPtr->start);
    json_initJsonParser(&monSetParser);
    memset(&monSetTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
    if ((ret = json_parseJson(&monSetParser, (const char *)monSetStr, monSetTokens, MAX_JSON_TOKEN_SIZE)) < 0) {
        printf("[%s] can't parse monSet(%s) ret(%d)\n", __func__, monSetStr, ret);
        return -1;
    }

    array_num = tokenPtr->size;
    if (array_num > max_item || array_num <= 0) {
        printf("[%s] key(%s) array number(%d) is over %d\n", __func__, key, array_num, max_item);
        return -1;
    }

    char        monSetArrayStr[HTTPF_MSG_BUFSIZE];
    JsonToken   monSetArrayTokens[MAX_JSON_TOKEN_SIZE];
    JsonParser  monSetArrayParser;

    for (i=0; i < array_num; i++) {
        if ((tokenPtr = json_object_array_get_idx_new(monSetStr, monSetTokens, i)) == NULL) {
            printf("[%s] can't get monSetArrayStr[%d] from (%s) ret(%d)\n", __func__, i, monSetStr, ret);
            return -1;
        }
		memset(monSetArrayStr, 0x00, HTTPF_MSG_BUFSIZE);
        strncpy(monSetArrayStr, &monSetStr[tokenPtr->start], tokenPtr->end - tokenPtr->start);
        json_initJsonParser(&monSetArrayParser);
        memset(&monSetArrayTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
        if ((ret = json_parseJson(&monSetArrayParser, (const char *)monSetArrayStr, monSetArrayTokens, MAX_JSON_TOKEN_SIZE)) < 0) {
            printf("[%s] can't parse monSetArray(%s) ret(%d)\n", __func__, monSetArrayStr, ret);
            return -1;
        }

		printf("\n\n\n###############################################\nmonSetArrayStr:[%s]\n", monSetArrayStr);

		for (j=0; j < MAX_JSON_TOKEN_SIZE; j++) {
			if (monSetArrayTokens[j].start == 0 && monSetArrayTokens[j].end == 0) break;
	
			strncpy(bb, &monSetArrayStr[monSetArrayTokens[j].start], monSetArrayTokens[j].end - monSetArrayTokens[j].start);
			bb[monSetArrayTokens[j].end - monSetArrayTokens[j].start] = 0;
			printf("j=%d %d %d %d %d [%s]\n", 
					j, monSetArrayTokens[j].type, monSetArrayTokens[j].start, monSetArrayTokens[j].end, monSetArrayTokens[j].size, bb);

		}
		printf("####################################################\n");

        mon_set_ptr = &mon_set[i];
        validity_ptr= &mon_set_ptr->validity;
        rch_mon_ptr = &mon_set_ptr->rch_mon;
        loc_mon_ptr = &mon_set_ptr->loc_mon;
        grp_mon_ptr = &mon_set_ptr->grp_mon;

        // monitorType (Mandatory)
        if (get_string_from_json_new(monSetArrayStr, monSetArrayTokens, "monitorType", mon_set_ptr->mon_type, MAX_SCS_MON_TYPE_LEN) < 0) {
            printf("[%s] can't get(%dth in %d %s) data from json[%s] in [%s]\n", __func__, i, array_num, "monitorType", monSetArrayStr, monSetStr);
            return -1;
        } else {
            printf("[DATA] %s[%d]: [%s]\n", "monitorType", i, mon_set_ptr->mon_type);
		}

        // validity (Mandatory)
		if ( get_validity_from_json_new(monSetArrayStr, monSetArrayTokens, "validity", validity_ptr) < 0) {
        	printf("can't get(%s) data from json\n", "validity");
        	return;
    	}

        // reportNumber (Mandatory)
        if (get_int_from_json_new(monSetArrayStr, monSetArrayTokens, "reportNumber", &mon_set_ptr->report_num) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "reportNumber");
            return -1;
        } else {
            printf("[DATA] %s[%d]: [%d]\n", "reportNumber", i, mon_set_ptr->report_num);
        }

        // chargingNumber (Mandatory)
        if (get_string_from_json_new(monSetArrayStr, monSetArrayTokens, "chargingNumber", mon_set_ptr->charging_num, MAX_SCS_CHARGE_NUM_LEN) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "chargingNumber");
            return -1;
        } else {
            printf("[DATA] %s[%d]: [%s]\n", "chargingNumber", i, mon_set_ptr->charging_num);
        }

        // set of reachableMonitor (Optional)
    	char        rechMonStr[HTTPF_MSG_BUFSIZE];
    	JsonToken   rechMonTokens[MAX_JSON_TOKEN_SIZE];
    	JsonParser  rechMonParser;

    	if ((tokenPtr = json_object_object_get_new(monSetArrayStr, monSetArrayTokens, "reachableMonitor")) == NULL) {
        	printf("can't parse reachableMonitor\n");
        	return;
    	}
		memset(rechMonStr, 0x00, HTTPF_MSG_BUFSIZE);
    	strncpy(rechMonStr, &monSetArrayStr[tokenPtr->start], tokenPtr->end - tokenPtr->start);
    	json_initJsonParser(&rechMonParser);
    	memset(&rechMonTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
    	if ((ret = json_parseJson(&rechMonParser, (const char *)rechMonStr, rechMonTokens, MAX_JSON_TOKEN_SIZE)) < 0) {
        	printf("[%s] can't parse reachableMonitor(%s) ret(%d)\n", __func__, rechMonStr, ret);
        	return;
		}

        // eventType (Mandatory)
        if ((evt_array_num = get_evttype_array_from_json_new(rechMonStr, rechMonTokens, "eventType", &rch_mon_ptr->event_type[0], MAX_SCS_EVT_TYPE_NUM, MAX_SCS_EVT_TYPE_LEN)) <= 0) {
        	printf("[%s] can't get eventType from json ret=%d\n", __func__, evt_array_num);
        	return;
        }
        printf("[DATA] eventType[%d]:[%s][%s]\n", i, rch_mon_ptr->event_type[0], rch_mon_ptr->event_type[1]);
        rch_mon_ptr->event_number = evt_array_num;

        // maximumLatency (Mandatory)
        if (get_int_from_json_new(rechMonStr, rechMonTokens, "maximumLatency", &rch_mon_ptr->max_latency) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "maximumLatency");
            return -1;
		}
        printf("[DATA] maximumLatency[%d]:[%d]\n", i, rch_mon_ptr->max_latency);

        // maximumResponse (Mandatory)
        if (get_int_from_json_new(rechMonStr, rechMonTokens, "maximumResponse", &rch_mon_ptr->max_resp) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "maximumResponse");
            return -1;
        }
        printf("[DATA] maximumResponse[%d]:[%d]\n", i, rch_mon_ptr->max_resp);
        // end of reachableMonitor

        // set of associationMonitor (Optional)
        if ((ass_array_num = get_assomon_array_from_json_new(monSetArrayStr, monSetArrayTokens, "associationMonitor", mon_set_ptr->ass_mon, MAX_SCS_ASS_TYPE_NUM, MAX_SCS_ASSO_MON_LEN)) <= 0) {
            printf("[%s] can't get associationMonitor from json ret=%d\n", __func__, ass_array_num);
            return -1;
        }
		printf("[DATA] associationMonitor[%d]:[%s][%s]\n", i, mon_set_ptr->ass_mon[0], mon_set_ptr->ass_mon[1]);

        // set of locationMonitor (Optional)
    	char        locMonStr[HTTPF_MSG_BUFSIZE];
    	JsonToken   locMonTokens[MAX_JSON_TOKEN_SIZE];
    	JsonParser  locMonParser;

    	if ((tokenPtr = json_object_object_get_new(monSetArrayStr, monSetArrayTokens, "locationMonitor")) == NULL) {
        	printf("can't parse locationMonitor\n");
        	return -1;
    	}
		memset(locMonStr, 0x00, HTTPF_MSG_BUFSIZE);
    	strncpy(locMonStr, &monSetArrayStr[tokenPtr->start], tokenPtr->end - tokenPtr->start);
    	json_initJsonParser(&locMonParser);
    	memset(&locMonTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
    	if ((ret = json_parseJson(&locMonParser, (const char *)locMonStr, locMonTokens, MAX_JSON_TOKEN_SIZE)) < 0) {
        	printf("[%s] can't parse locationMonitor(%s) ret(%d)\n", __func__, locMonStr, ret);
        	return -1;
    	}

        // locationType (Mandatory)
        if (get_string_from_json_new(locMonStr, locMonTokens, "locationType", loc_mon_ptr->loc_type, MAX_SCS_LOC_TYPE_LEN) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "locationType");
            return -1;
        }
		printf("[DATA] locationType[%d]:[%s]\n", i, loc_mon_ptr->loc_type);

        // accuracy (Mandatory)
        if (get_string_from_json_new(locMonStr, locMonTokens, "accuracy", loc_mon_ptr->accuracy, MAX_SCS_ACCURACY_LEN) < 0) {
            printf("[%s] can't get(%s) data from json\n", __func__, "accuracy");
            return -1;
        }
		printf("[DATA] accuracy[%d]:[%s]\n", i, loc_mon_ptr->accuracy);
        // end of locationMonitor

        // set of groupMonitor (Optional)
    	char        grpMonStr[HTTPF_MSG_BUFSIZE];
    	JsonToken   grpMonTokens[MAX_JSON_TOKEN_SIZE];
    	JsonParser  grpMonParser;

    	if ((tokenPtr = json_object_object_get_new(monSetArrayStr, monSetArrayTokens, "groupMonitor")) == NULL) {
        	printf("can't parse groupMonitor\n");
        	return -1;
    	}
		memset(grpMonStr, 0x00, HTTPF_MSG_BUFSIZE);
    	strncpy(grpMonStr, &monSetArrayStr[tokenPtr->start], tokenPtr->end - tokenPtr->start);
    	json_initJsonParser(&grpMonParser);
    	memset(&grpMonTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
    	if ((ret = json_parseJson(&grpMonParser, (const char *)grpMonStr, grpMonTokens, MAX_JSON_TOKEN_SIZE)) < 0) {
        	printf("[%s] can't parse groupMonitor(%s) ret(%d)\n", __func__, grpMonStr, ret);
        	return -1;
    	}

        // groupType
        if (get_string_from_json_new(grpMonStr, grpMonTokens, "groupType", grp_mon_ptr->group_type, MAX_SCS_GRP_TYPE_LEN) < 0) {
        	printf("[%s] can't get(%s) data from json\n", __func__, "groupType");
        	return -1;
        }
		printf("[DATA] groupType[%d]:[%s]\n", i, grp_mon_ptr->group_type);

        // groupValue
        if (get_string_from_json_new(grpMonStr, grpMonTokens, "groupValue", grp_mon_ptr->group_value, MAX_SCS_GRP_VAL_LEN) < 0) {
        	printf("[%s] can't get(%s) data from json\n", __func__, "groupValue");
        	return -1;
        }
		printf("[DATA] groupValue[%d]:[%s]\n", i, grp_mon_ptr->group_value);
        // end of groupMonitor
    }

    return array_num;
}


void main_cp_info()
//void main()
{
	int ret, i, j, array_num, size;
	char json_str[HTTPF_MSG_BUFSIZE], bb[HTTPF_MSG_BUFSIZE], cc[HTTPF_MSG_BUFSIZE];
	JsonToken tokens[MAX_JSON_TOKEN_SIZE];


	//sprintf(a, "{\"nidd:info\" : { \"scsRefereneId\": \"89ABCDEF\", \"scsId\":\"scs01\", \"address\": [\"tel:+821022223333\",\"tel:+821022224444\"], \"validity\": { \"startTime\": \"2016-06-23T09:00:00\", \"expiryTime\": \"2017-06-23T09:00:00\" }}}");
	sprintf(json_str, "{ \"cp:info\" : { \"scsRefereneId\": \"23456789\", \"scsId\":\"scs01\", \"address\": [ \"tel:+821023456789\" ], \"validity\": { \"startTime\": \"2016-06-23T09:00:00\", \"expiryTime\": \"2017-06-23 09:00:00\" }, \"parameterSet\": { \"periodicCommunication\": { \"commIndicator\": \"Periodic\", \"duration\": 300, \"interval\": 7200 }, \"scheduledCommunication\": [{ \"timeOfDayStart\": \"0\", \"timeOfDayEnd\": \"18000\", \"dayOfWeekMask\": \"1111100\", \"timezoneFlag\": \"2\" }, { \"timeOfDayStart\": \"68400\", \"timeOfDayEnd\": \"86400\", \"dayOfWeekMask\": \"0000011\", \"timezoneFlag\": \"1\" }], \"stationary\": \"Mobile\" } } } ");

	printf("json_str=[%s]%d %d\n\n", json_str, JSON_STRING, JSON_OBJECT);

	JsonParser parser;
	json_initJsonParser(&parser);

	memset(&tokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
	ret = json_parseJson(&parser, (const char *)json_str, tokens, MAX_JSON_TOKEN_SIZE);
	printf("khlret=%d %d %d %d\n", ret, parser.pos, parser.toknext, parser.toksuper);

	size = parser.toknext;
	// Resource Name
	i = 1;
	strncpy(bb, &json_str[tokens[i].start], tokens[i].end - tokens[i].start);
	bb[tokens[i].end - tokens[i].start] = 0;
	printf("resource name [%s]\n", bb);

	// scsRefereneId (Mandatory)
 	if ( get_string_from_json_new(json_str, tokens, "scsRefereneId", cc, 32) < 0) {
		printf("can't get(%s) data from json\n", "scsRefereneId");
		return;
	} else {
		printf("[DATA] %s:[%s]\n", "scsRefereneId", cc);
	}

	// address (Mandatory)
	char	address[10][33];
	if ((array_num = get_address_array_from_json_new(json_str, tokens, "address", &address[0], 10, 32)) <= 0) {
        printf("[%s] can't get address from json ret=%d\n", __func__, array_num);
        return;
	} else {
		for (i=0; i<array_num; i++) printf("[DATA] %s[%d]:[%s]\n", "address", i, address[i]);
    }

	// validity (Optional)
	Scs_validity_t	validity;
	if ( get_validity_from_json_new(json_str, tokens, "validity", &validity) < 0) {
        printf("can't get(%s) data from json\n", "scsRefereneId");
        return;
    } else {
        printf("[DATA] %s:[%s]\n", "scsRefereneId", cc);
    }

	/* set of parameterSet (Mandatory)
     */
	char	paraSetStr[2048];
	JsonToken   paraSetTokens[MAX_JSON_TOKEN_SIZE], *tokenPtr;
	JsonParser paraSetParser;

    if ((tokenPtr = json_object_object_get_new(json_str, tokens, "parameterSet")) == NULL) {
        printf("can't parse parameterSet\n");
    }
	strncpy(paraSetStr, &json_str[tokenPtr->start], tokenPtr->end - tokenPtr->start);

	json_initJsonParser(&paraSetParser);
	memset(&paraSetTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
	ret = json_parseJson(&paraSetParser, (const char *)paraSetStr, paraSetTokens, MAX_JSON_TOKEN_SIZE);
    int paraSetTokenSize = paraSetParser.toknext;
	printf("[%s] paraSetTokenSize = %d\n", paraSetStr, paraSetTokenSize);

#if 0
	printf("\n\n\nparaSetTokens:\n");

	for (i=0; i < paraSetTokenSize; i++) {
		if (paraSetTokens[i].start == 0 && paraSetTokens[i].end == 0)
			break;
	
		strncpy(bb, &paraSetStr[paraSetTokens[i].start], paraSetTokens[i].end - paraSetTokens[i].start);
		bb[paraSetTokens[i].end - paraSetTokens[i].start] = 0;
		printf("i=%d %d %d %d %d [%s]\n", 
				i, paraSetTokens[i].type, paraSetTokens[i].start, paraSetTokens[i].end, paraSetTokens[i].size, bb);

	}
#endif

	// set of periodicCommunication (Mandatory)
	char		periodCommStr[2048];
	JsonToken   periodCommTokens[MAX_JSON_TOKEN_SIZE];
	JsonParser  periodCommParser;

    if ((tokenPtr = json_object_object_get_new(paraSetStr, paraSetTokens, "periodicCommunication")) == NULL) {
        printf("can't parse periodicCommunication\n");
    }
	strncpy(periodCommStr, &paraSetStr[tokenPtr->start], tokenPtr->end - tokenPtr->start);
	json_initJsonParser(&periodCommParser);
	memset(&periodCommTokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
	ret = json_parseJson(&periodCommParser, (const char *)periodCommStr, periodCommTokens, MAX_JSON_TOKEN_SIZE);
    int periodCommTokenSize = periodCommTokens->size;

	printf("[%s] periodCommTokenSize = %d\n", periodCommStr, periodCommTokenSize);

#if 0
	printf("\n\n\nperiodCommStr:\n");

	for (i=0; i < MAX_JSON_TOKEN_SIZE; i++) {
		if (periodCommTokens[i].start == 0 && periodCommTokens[i].end == 0)
			break;
	
		strncpy(bb, &periodCommStr[periodCommTokens[i].start], periodCommTokens[i].end - periodCommTokens[i].start);
		bb[periodCommTokens[i].end - periodCommTokens[i].start] = 0;
		printf("i=%d %d %d %d %d [%s]\n", 
				i, periodCommTokens[i].type, periodCommTokens[i].start, periodCommTokens[i].end, periodCommTokens[i].size, bb);

	}
#endif

	// commIndicator (Mandatory)
	char	comm_indicator[1024];
   	if (get_string_from_json_new(periodCommStr, periodCommTokens, "commIndicator", comm_indicator, MAX_SCS_COMM_IND_LEN) < 0) {
        printf("[%s] can't get(%s) data from json\n", __func__, "commIndicator");
    } else {
        printf("[DATA] %s=[%s]\n", "commIndicator", comm_indicator);
	}
 
	// duration (Mandatory)
	int	duration;
	if (get_int_from_json_new(periodCommStr, periodCommTokens, "duration", &duration) < 0) {
        printf("[%s] can't get(%s) data from json\n", __func__, "duration");
    } else {
        printf("[DATA] %s=[%d]\n", "duration", duration);
    }

	// interval (Mandatory)
	int	interval;
	if (get_int_from_json_new(periodCommStr, periodCommTokens, "interval", &interval) < 0) {
        printf("[%s] can't get(%s) data from json\n", __func__, "interval");
    } else {
        printf("[DATA] %s=[%d]\n", "interval", interval);
    }
	// end of periodicCommunication

	// set of scheduledCommunication (Mandatory)
	Scs_schedule_comm_t sched_comm_ptr[MAX_SCS_SCHEDULE_COMM_NUM];
	if ((array_num = get_sched_comm_array_from_json_new(paraSetStr, paraSetTokens, "scheduledCommunication", sched_comm_ptr, MAX_SCS_SCHEDULE_COMM_NUM)) <= 0) {
        printf("[%s] can't get scheduledCommunication from paraSet ret=%d\n", __func__, array_num);
        return;
    } else {
        printf("[%s] num of scheduledCommunication from json ret=%d\n", __func__, array_num);
	}
    int sched_comm_number = array_num;


	// stationary (Mandatory)
	char	stationary[1024];
    if (get_string_from_json_new(paraSetStr, paraSetTokens, "stationary", &stationary[0], MAX_SCS_STATIONARY_LEN) < 0) {
        printf("[%s] can't get(%s) data from json\n", __func__, "stationary");
    } else {
        printf("[DATA] %s=[%s]\n", "stationary", stationary);
    }

	printf("\n\n");
	for (i=0; i < parser.toknext; i++) {
		if (tokens[i].start == 0 && tokens[i].end == 0)
			break;
	
		strncpy(bb, &json_str[tokens[i].start], tokens[i].end - tokens[i].start);
		bb[tokens[i].end - tokens[i].start] = 0;
		printf("i=%d %d %d %d %d [%s]\n", 
				i, tokens[i].type, tokens[i].start, tokens[i].end, tokens[i].size, bb);

#if 0
		if (!strcmp(bb, "scsRefereneId")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		} else if (!strcmp(bb, "scsId")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		} else if (!strcmp(bb, "address")) {
			i++;
			array_num = tokens[i].size;
			for (j=0; j<array_num; j++) {
				i++;
				strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
				cc[tokens[i].end - tokens[i].start] = 0;

				printf("%s=%s\n", bb, cc);
			}
		} else if (!strcmp(bb, "startTime")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		} else if (!strcmp(bb, "expiryTime")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		}
#endif
	}
}

//void main()
void main_evt_info()
{	
	int ret, i, j, array_num, size;
	char json_str[HTTPF_MSG_BUFSIZE], bb[HTTPF_MSG_BUFSIZE], cc[HTTPF_MSG_BUFSIZE];
	JsonToken tokens[MAX_JSON_TOKEN_SIZE];


	sprintf(json_str, "{ \"event:info\" : { \"scsRefereneId\": \"23456789\", \"scsId\":\"scs01\", \"address\": [ \"tel:+821023456789\" ], \"monitorSet\" : [{ \"monitorType\" : \"lossofConnectivity\", \"validity\": { \"startTime\": \"2016-06-23 09:00:00\", \"expiryTime\": \"2017-06-23 09:00:00\" }, \"reportNumber\": 10, \"chargingNumber\": \"821023456789\", \"reachableMonitor\": { \"eventType\": [ \"SMS\", \"DATA\" ], \"maximumLatency\" : 1000, \"maximumResponse\" : 2000 }, \"associationMonitor\": [ \"IMSI\", \"IMEISV\" ], \"locationMonitor\": { \"locationType\": \"current\", \"accuracy\": \"ecgi\" }, \"groupMonitor\": { \"groupType\": \"ecgi\", \"groupValue\": \"2cab\" } }, { \"monitorType\" : \"ueReachable\", \"validity\": { \"startTime\": \"2017-01-01 00:00:00\", \"expiryTime\": \"2017-12-31 24:00:00\" }, \"reportNumber\": 11, \"chargingNumber\": \"821023456789\", \"reachableMonitor\": { \"eventType\": [ \"SMS\" ], \"maximumLatency\" : 1001, \"maximumResponse\" : 2001 }, \"associationMonitor\": [ \"IMSI\" ], \"locationMonitor\": { \"locationType\": \"last\", \"accuracy\": \"enb\" }, \"groupMonitor\": { \"groupType\": \"enbId\", \"groupValue\": \"enb01\" } }], \"destinationAddress\": \"192.100.101.101:9002\" } }");
	//sprintf(json_str, "{ \"event:info\" : { \"scsRefereneId\": \"12345678\", \"scsId\":\"scs01\", \"address\": [ \"tel:+821012345678\" ], \"monitorSet\" : [{ \"monitorType\" : \"lossofConnectivity\", \"validity\": { \"startTime\": \"2016-06-23 09:00:00\", \"expiryTime\": \"2017-06-23 09:00:00\" }, \"reportNumber\": 10, \"chargingNumber\": \"821022223333\", \"reachableMonitor\": { \"eventType\": [ \"SMS\", \"DATA\" ], \"maximumLatency\" : 1000, \"maximumResponse\" : 2000 }, \"associationMonitor\": [ \"IMSI\", \"IMEISV\" ], \"locationMonitor\": { \"locationType\": \"current\", \"accuracy\": \"ecgi\" }, \"groupMonitor\": { \"groupType\": \"ecgi\", \"groupValue\": \"2cab\" } }], \"destinationAddress\": \"192.100.101.101:9002\" } }");

	printf("json_str=[%s]%d %d\n\n", json_str, JSON_STRING, JSON_OBJECT);

	JsonParser parser;
	json_initJsonParser(&parser);

	memset(&tokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);
	ret = json_parseJson(&parser, (const char *)json_str, tokens, MAX_JSON_TOKEN_SIZE);
	printf("khlret=%d %d %d %d\n", ret, parser.pos, parser.toknext, parser.toksuper);

	size = parser.toknext;
	// Resource Name
	i = 1;
	strncpy(bb, &json_str[tokens[i].start], tokens[i].end - tokens[i].start);
	bb[tokens[i].end - tokens[i].start] = 0;
	printf("resource name [%s]\n", bb);

	// scsRefereneId (Mandatory)
 	if ( get_string_from_json_new(json_str, tokens, "scsRefereneId", cc, 32) < 0) {
		printf("can't get(%s) data from json\n", "scsRefereneId");
		return;
	} else {
		printf("[DATA] %s:[%s]\n", "scsRefereneId", cc);
	}

	// address (Mandatory)
	char	address[10][33];
	if ((array_num = get_address_array_from_json_new(json_str, tokens, "address", &address[0], 10, 32)) <= 0) {
        printf("[%s] can't get address from json ret=%d\n", __func__, array_num);
        return;
	} else {
		for (i=0; i<array_num; i++) printf("[DATA] %s[%d]:[%s]\n", "address", i, address[i]);
    }

	/* set of monitorSet (Mandatory)
	 */
	Scs_mon_set_t       mon_set_ptr[MAX_SCS_MON_SET_NUM];
	if ((array_num = get_monset_array_from_json_new(json_str, tokens, "monitorSet", mon_set_ptr, MAX_SCS_MON_SET_NUM)) <= 0) {
		printf("[%s] can't get monitor set from json ret=%d\n", __func__, array_num);
		return;
	} else {
		printf("[%s] get monitor set from json ret=%d\n", __func__, array_num);
	}
	//evt_info->monset_number = array_num;

	printf("\n\n");
	for (i=0; i < parser.toknext; i++) {
		if (tokens[i].start == 0 && tokens[i].end == 0)
			break;
	
		strncpy(bb, &json_str[tokens[i].start], tokens[i].end - tokens[i].start);
		bb[tokens[i].end - tokens[i].start] = 0;
		printf("i=%d %d %d %d %d [%s]\n", 
				i, tokens[i].type, tokens[i].start, tokens[i].end, tokens[i].size, bb);
	}
}


void main_nidd_info(int print_flag)
//void main()
{
	int ret, i, j, array_num, size;
	char json_str[HTTPF_MSG_BUFSIZE], bb[HTTPF_MSG_BUFSIZE], cc[HTTPF_MSG_BUFSIZE];
	JsonToken tokens[MAX_JSON_TOKEN_SIZE];

	sprintf(json_str, "{\"nidd:info\" : { \"scsRefereneId\": \"89ABCDEF\", \"scsId\":\"scs01\", \"address\": [\"tel:+821022223333\",\"tel:+821022224444\"], \"validity\": { \"startTime\": \"2016-06-23T09:00:00\", \"expiryTime\": \"2017-06-23T09:00:00\" }}}");

	if (print_flag) printf("json_str=[%s]%d %d\n\n", json_str, JSON_STRING, JSON_OBJECT);

	JsonParser parser;
	json_initJsonParser(&parser);

	memset(&tokens[0], 0, sizeof(JsonToken)*MAX_JSON_TOKEN_SIZE);

    // parser, json_str ---> tokens 추출 
	ret = json_parseJson(&parser, (const char *)json_str, tokens, MAX_JSON_TOKEN_SIZE);
	if (print_flag) printf("khlret=%d %d %d %d\n", ret, parser.pos, parser.toknext, parser.toksuper);

	size = parser.toknext;
	// Resource Name
	i = 1;
	strncpy(bb, &json_str[tokens[i].start], tokens[i].end - tokens[i].start);
	bb[tokens[i].end - tokens[i].start] = 0;
	if (print_flag) printf("resource name [%s]\n", bb);

	// scsId (Mandatory)
 	if ( get_string_from_json_new(json_str, tokens, "scsId", cc, MAX_SCS_ID_LEN) < 0) {
		printf("can't get(%s) data from json\n", "scsId");
		return;
	} else {
		if (print_flag) printf("[DATA] %s:[%s]\n", "scsId", cc);
	}

	// scsRefereneId (Mandatory)
 	if ( get_string_from_json_new(json_str, tokens, "scsRefereneId", cc, MAX_SCS_REF_ID_LEN) < 0) {
		printf("can't get(%s) data from json\n", "scsRefereneId");
		return;
	} else {
		if (print_flag) printf("[DATA] %s:[%s]\n", "scsRefereneId", cc);
	}

	// address (Mandatory)
	char	address[10][33];
	if ((array_num = get_address_array_from_json_new(json_str, tokens, "address", &address[0], 10, 32)) <= 0) {
        printf("[%s] can't get address from json ret=%d\n", __func__, array_num);
        return;
	} else {
		if (print_flag) for (i=0; i<array_num; i++) printf("[DATA] %s[%d]:[%s]\n", "address", i, address[i]);
    }

	// validity (Optional)
	Scs_validity_t	validity;
	if ( get_validity_from_json_new(json_str, tokens, "validity", &validity) < 0) {
        printf("can't get(%s) data from json\n", "scsRefereneId");
        return;
    } else {
        if (print_flag) printf("[DATA] %s:[%s]\n", "scsRefereneId", cc);
    }

#if 0
	printf("\n\n");
	for (i=0; i < parser.toknext; i++) {
		if (tokens[i].start == 0 && tokens[i].end == 0)
			break;
	
		strncpy(bb, &json_str[tokens[i].start], tokens[i].end - tokens[i].start);
		bb[tokens[i].end - tokens[i].start] = 0;
		printf("i=%d %d %d %d %d [%s]\n", 
				i, tokens[i].type, tokens[i].start, tokens[i].end, tokens[i].size, bb);

#if 0
		if (!strcmp(bb, "scsRefereneId")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		} else if (!strcmp(bb, "scsId")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		} else if (!strcmp(bb, "address")) {
			i++;
			array_num = tokens[i].size;
			for (j=0; j<array_num; j++) {
				i++;
				strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
				cc[tokens[i].end - tokens[i].start] = 0;

				printf("%s=%s\n", bb, cc);
			}
		} else if (!strcmp(bb, "startTime")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		} else if (!strcmp(bb, "expiryTime")) {
			i++;
			strncpy(cc, &a[tokens[i].start], tokens[i].end - tokens[i].start);
			cc[tokens[i].end - tokens[i].start] = 0;

			printf("%s=%s\n", bb, cc);
		}
#endif
	}
#endif
}

char commlib_microTimeStamp[32];
char *commlib_printMicrosec (void)
{
    struct timeval  now;
    struct tm       *pLocalTime;
    char            tmp[32];

    gettimeofday (&now, NULL);
    if ((pLocalTime = localtime(&now.tv_sec)) == NULL) {
        strcpy(commlib_microTimeStamp,"");
    } else {
        strftime(tmp, 32, "%T", pLocalTime);
        sprintf (commlib_microTimeStamp, "%s.%06d", tmp, (int)(now.tv_usec));
    }

    return commlib_microTimeStamp;

} //----- End of commlib_printMicrosec -----//

void main()
{
	int i;

	printf("START:%s\n", commlib_printMicrosec());

	for (i=0; i < 10000; i++) main_nidd_info(0);

	printf("END:%s\n", commlib_printMicrosec());
}



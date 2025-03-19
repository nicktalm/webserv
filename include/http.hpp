#pragma once

namespace Hallo
{
	int get_idk();
}

int Hallo::get_idk(){

}

typedef enum e_http_codes 
{
	OK = 200,
	ERROR = 404,
} t_http_codes;

int status_code = 404;

t_http_codes code = ERROR;
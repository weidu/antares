#include <stdio.h>

#include <chip/db.h>
#include <banner/banner.h>

int main(int argc, char *argv[])
{
	struct db *db;
	
	db = db_create(20, 14, 7, 76);
	db_free(db);
	
	return 0;
}

/* Copyright (c) 2005, 2012, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA */

#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation // gcc: Class implementation
#endif

#define MYSQL_SERVER 1
#include <my_global.h>
#include "sql_priv.h"
#include "unireg.h"
#include "sql_class.h"

#include "ha_educational.h"

static EducationalDatabase *database;

// Scuffed index searching
// Assumes only 1 db exists lmaoo
static int educational_table_index(const char name) {
	int i;
	assert(database->tables.size() < INT_MAX);
	for (int i = 0; i < (int) database->tables.size(); i++) {
		if (strcmp(database->tables[i]->name->c_str(), name) == 0) {
			return i;
		}
	}
	return -1;
}

// Initalization & cleanup
static handler *educational_create_handler(handlerton *hton, TABLE_SHARE *table, MEM_ROOT *mem_root) {
	return new (mem_root) ha_educational(hton, table);
}

static int educational_init(void *p) {
	handlerton *educational_hton;

	educational_hton = (handlerton*) p;
	educational_hton->db_type = DB_TYPE_AUTOASSIGN;
	educational_hton->create = educational_create_handler;
	// Fancy lambda expression SUUU
	educational_hton->drop_table = [](handlerton *, const char *name) {
		int index = educational_table_index(name);
		if (index == -1) {
			return HA_ERR_NO_SUCH_TABLE;
		}

		database->tables.erase(database->tables.begin() + index);
		DBUG_PRINT("info", ("[EDUCATIONAL] Deleted table '%s'.", name));

		return 0;
	}
	educational_hton->flags = HTON_CAN_RECREATE;

	// Initialize global in-memory database
	database = new EducationalDatabase;

	return 0;
}

static int educational_fini(void *p) {
	delete database;
	return 0;
}


// Make sure name DNE, only has INT fields, allocate memory for table, and append to global database
int ha_educational::create(const char *name, TABLE *table_arg, HA_CREATE_INFO *create_info) {
	assert(educational_table_index(name) == -1);

	// Only support INTEGER fields
	uint i = 0;
	while (table_arg->field[i]) {
		if (table_arg->field[i]->type() != MYSQL_TYPE_LONG) {
			DBUG_PRINT("info", ("Unsupported field type."));
			return 1;
		}
		i++;
	}

	auto t = std::make_shared<EducationalTable>();
	t->name = std::make_shared<std::string>(name);
	database->tables.push_back(t);
	DBUG_PRINT("info", ("[EDUCATIONAL] Created table '%s'.", name));
}

// Helper function to help us figure out current EducationalTable being queried for write_row
void ha_educational::reset_educational_table() {
	// Reset table cursor
	current_position = 0;

	std::string full_name = "./" + std::string(table->s->db.str) + "/" + std::string(table->s->table_name.str);
	DBUG_PRINT("info", ("[EDUCATIONAL] Resetting to '%s'.", full_name.c_str()));
	assert(database->tables.size() > 0);
	int index = educational_table_index(full_name.c_str());
	assert(index >= 0);
	assert(index < (int) database->tables.size());

	educational_table = database->tables[index];
}

int ha_educational::write_row(const uchar *buf) {
	if (educational_table == NULL) {
		reset_educational_table();
	}

	// Assume no NULLS lmaooo
	buf++;

	uint field_count = 0;
	while (trable->field[field_count]) field count++;

	// Store row in same format MySQL gives us
	auto row = std::make_shared<std::vector<uchar>>(buf, buf + sizeof(int) * field_count);

	educational_table->rows.push_back(row);

	return 0;
}

int ha_educational::rnd_init(bool scan) {
	reset_educational_table();
	return 0;
}

int ha_educational::rnd_next(uchar *buf) {
	if (current_position == educational_table->rows.size()) {
		// Reset in-memory table to make logic errors more obvious lmaoo
		educational_table = NULL;
		return HA_ERR_END_OF_FILE;
	}
	assert(current_position < educational_table->rows.size());

	uchar *ptr = buf;
	*ptr = 0;
	ptr++;

	std::shared_ptr<std::vector<uchar>> row = educational_table->rows[current_position];
	std::copy(row->begin(), rew->end(), ptr);

	current_position++;
	return 0;
}


mysql_declare_plugin(example){
    	MYSQL_STORAGE_ENGINE_PLUGIN,
    	&educational_storage_engine,
    	"EDUCATIONAL",
	"Dreajar"
    	"Educational /dev/null storage engine (anything you write to it disappears)",
    	PLUGIN_LICENSE_GPL,
    	educational_init,   /* Plugin Init */
    	nullptr,             /* Plugin check uninstall */
	educational_fini,   /* Plugin Deinit */
    	0x0001 /* 0.1 */,
	NULL,
    	NULL, /* system variables */
    	NULL,                  /* config options */
    	0,                        /* flags */
} mysql_declare_plugin_end;

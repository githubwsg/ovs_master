/* Copyright (c) 2009 Nicira Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OVSDB_OVSDB_H
#define OVSDB_OVSDB_H 1

#include "compiler.h"
#include "hmap.h"
#include "list.h"
#include "shash.h"

struct json;
struct uuid;

/* Database schema. */
struct ovsdb_schema {
    char *name;
    char *comment;
    struct shash tables;        /* Contains "struct ovsdb_table_schema *"s. */
};

struct ovsdb_schema *ovsdb_schema_create(const char *name,
                                         const char *comment);
void ovsdb_schema_destroy(struct ovsdb_schema *);

struct ovsdb_error *ovsdb_schema_from_file(const char *file_name,
                                           struct ovsdb_schema **)
    WARN_UNUSED_RESULT;
struct ovsdb_error *ovsdb_schema_from_json(struct json *,
                                           struct ovsdb_schema **)
    WARN_UNUSED_RESULT;
struct json *ovsdb_schema_to_json(const struct ovsdb_schema *);

/* Database. */
struct ovsdb {
    struct ovsdb_schema *schema;
    struct ovsdb_file *file;    /* Disk file (null for in-memory db). */
    struct shash tables;        /* Contains "struct ovsdb_table *"s. */

    /* Triggers. */
    struct list triggers;       /* Contains "struct ovsdb_trigger"s. */
    bool run_triggers;
};

struct ovsdb *ovsdb_create(struct ovsdb_file *, struct ovsdb_schema *);
struct ovsdb_error *ovsdb_open(const char *file_name, bool read_only,
                               struct ovsdb **)
    WARN_UNUSED_RESULT;
void ovsdb_destroy(struct ovsdb *);

struct ovsdb_error *ovsdb_from_json(const struct json *, struct ovsdb **)
    WARN_UNUSED_RESULT;
struct json *ovsdb_to_json(const struct ovsdb *);

struct ovsdb_table *ovsdb_get_table(const struct ovsdb *, const char *);

struct json *ovsdb_execute(struct ovsdb *, const struct json *params,
                           long long int elapsed_msec,
                           long long int *timeout_msec);

#endif /* ovsdb/ovsdb.h */

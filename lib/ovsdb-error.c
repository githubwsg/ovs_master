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

#include <config.h>

#include "ovsdb-error.h"

#include <inttypes.h>

#include "backtrace.h"
#include "dynamic-string.h"
#include "json.h"
#include "util.h"

struct ovsdb_error {
    const char *tag;            /* String for "error" member. */
    char *details;              /* String for "details" member. */
    char *syntax;               /* String for "syntax" member. */
    int errno_;                 /* Unix errno value, 0 if none. */
};

static struct ovsdb_error *
ovsdb_error_valist(const char *tag, const char *details, va_list args)
{
    struct ovsdb_error *error = xmalloc(sizeof *error);
    error->tag = tag ? tag : "ovsdb error";
    error->details = details ? xvasprintf(details, args) : NULL;
    error->syntax = NULL;
    error->errno_ = 0;
    return error;
}

struct ovsdb_error *
ovsdb_error(const char *tag, const char *details, ...)
{
    struct ovsdb_error *error;
    va_list args;

    va_start(args, details);
    error = ovsdb_error_valist(tag, details, args);
    va_end(args);

    return error;
}

struct ovsdb_error *
ovsdb_io_error(int errno_, const char *details, ...)
{
    struct ovsdb_error *error;
    va_list args;

    va_start(args, details);
    error = ovsdb_error_valist("I/O error", details, args);
    va_end(args);

    error->errno_ = errno_;

    return error;
}

struct ovsdb_error *
ovsdb_syntax_error(const struct json *json, const char *tag,
                   const char *details, ...)
{
    struct ovsdb_error *error;
    va_list args;

    va_start(args, details);
    error = ovsdb_error_valist(tag ? tag : "syntax error", details, args);
    va_end(args);

    if (json) {
        /* XXX this is much too much information in some cases */
        error->syntax = json_to_string(json, 0);
    }

    return error;
}

struct ovsdb_error *
ovsdb_wrap_error(struct ovsdb_error *error, const char *details, ...)
{
    va_list args;
    char *msg;

    va_start(args, details);
    msg = xvasprintf(details, args);
    va_end(args);

    if (error->details) {
        char *new = xasprintf("%s: %s", msg, error->details);
        free(error->details);
        error->details = new;
        free(msg);
    } else {
        error->details = msg;
    }

    return error;
}

struct ovsdb_error *
ovsdb_internal_error(const char *file, int line, const char *details, ...)
{
    struct ds ds = DS_EMPTY_INITIALIZER;
    struct backtrace backtrace;
    struct ovsdb_error *error;
    va_list args;

    ds_put_format(&ds, "%s:%d:", file, line);

    if (details) {
        ds_put_char(&ds, ' ');
        va_start(args, details);
        ds_put_format_valist(&ds, details, args);
        va_end(args);
    }

    backtrace_capture(&backtrace);
    if (backtrace.n_frames) {
        int i;

        ds_put_cstr(&ds, " (backtrace:");
        for (i = 0; i < backtrace.n_frames; i++) {
            ds_put_format(&ds, " 0x%08"PRIxPTR, backtrace.frames[i]);
        }
        ds_put_char(&ds, ')');
    }

    ds_put_format(&ds, " (%s %s%s)", program_name, VERSION, BUILDNR);

    error = ovsdb_error("internal error", "%s", ds_cstr(&ds));

    ds_destroy(&ds);

    return error;
}

void
ovsdb_error_destroy(struct ovsdb_error *error)
{
    if (error) {
        free(error->details);
        free(error->syntax);
        free(error);
    }
}

struct ovsdb_error *
ovsdb_error_clone(const struct ovsdb_error *old)
{
    if (old) {
        struct ovsdb_error *new = xmalloc(sizeof *new);
        new->tag = old->tag;
        new->details = old->details ? xstrdup(old->details) : NULL;
        new->syntax = old->syntax ? xstrdup(old->syntax) : NULL;
        new->errno_ = old->errno_;
        return new;
    } else {
        return NULL;
    }
}

static const char *
ovsdb_errno_string(int error)
{
    return error == EOF ? "unexpected end of file" : strerror(error);
}

struct json *
ovsdb_error_to_json(const struct ovsdb_error *error)
{
    struct json *json = json_object_create();
    json_object_put_string(json, "error", error->tag);
    if (error->details) {
        json_object_put_string(json, "details", error->details);
    }
    if (error->syntax) {
        json_object_put_string(json, "syntax", error->syntax);
    }
    if (error->errno_) {
        json_object_put_string(json, "io-error",
                               ovsdb_errno_string(error->errno_));
    }
    return json;
}

char *
ovsdb_error_to_string(const struct ovsdb_error *error)
{
    struct ds ds = DS_EMPTY_INITIALIZER;
    if (error->syntax) {
        ds_put_format(&ds, "syntax \"%s\": ", error->syntax);
    }
    ds_put_cstr(&ds, error->tag);
    if (error->details) {
        ds_put_format(&ds, ": %s", error->details);
    }
    if (error->errno_) {
        ds_put_format(&ds, " (%s)", ovsdb_errno_string(error->errno_));
    }
    return ds_steal_cstr(&ds);
}

const char *
ovsdb_error_get_tag(const struct ovsdb_error *error)
{
    return error->tag;
}

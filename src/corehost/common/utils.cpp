// Copyright (c) .NET Foundation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "utils.h"
#include "trace.h"

bool coreclr_exists_in_dir(const pal::string_t& candidate)
{
    pal::string_t test(candidate);
    append_path(&test, LIBCORECLR_NAME);
    trace::verbose(_X("Checking if CoreCLR path exists=[%s]"), test.c_str());
    return pal::file_exists(test);
}

bool ends_with(const pal::string_t& value, const pal::string_t& suffix)
{
    return suffix.length() <= value.length() &&
        (0 == value.compare(value.length() - suffix.length(), suffix.length(), suffix));
}

bool starts_with(const pal::string_t& value, const pal::string_t& prefix)
{
    return value.find(prefix.c_str(), 0, prefix.length()) == 0;
}

void append_path(pal::string_t* path1, const pal::char_t* path2)
{
    if (pal::is_path_rooted(path2))
    {
        path1->assign(path2);
    }
    else
    {
        if (path1->empty() || path1->back() != DIR_SEPARATOR)
        {
            path1->push_back(DIR_SEPARATOR);
        }
        path1->append(path2);
    }
}

pal::string_t get_executable(const pal::string_t& filename)
{
    pal::string_t result(filename);

    if (ends_with(result, _X(".exe")))
    {
        // We need to strip off the old extension
        result.erase(result.length() - 4);
    }

    return result;
}

pal::string_t strip_file_ext(const pal::string_t& path)
{
    if (path.empty())
    {
        return path;
    }
    return path.substr(0, path.rfind(_X('.')));
}

pal::string_t get_filename_without_ext(const pal::string_t& path, const pal::char_t dir_sep)
{
    if (path.empty())
    {
        return path;
    }

    size_t name_pos = path.find_last_of(dir_sep);
    if (name_pos == pal::string_t::npos)
    {
        return path.substr(0, path.find_last_of(_X('.')));
    }
    size_t dot_pos = path.find_last_of(_X('.'), name_pos);
    return path.substr(name_pos + 1, dot_pos);
}

pal::string_t get_filename(const pal::string_t& path)
{
    if (path.empty())
    {
        return path;
    }

    auto name_pos = path.find_last_of(DIR_SEPARATOR);
    if (name_pos == pal::string_t::npos)
    {
        return path;
    }

    return path.substr(name_pos + 1);
}

pal::string_t get_directory(const pal::string_t& path)
{
    // Find the last dir separator
    auto path_sep = path.find_last_of(DIR_SEPARATOR);
    if (path_sep == pal::string_t::npos)
    {
        return pal::string_t(path);
    }

    return path.substr(0, path_sep);
}

void replace_char(pal::string_t* path, pal::char_t match, pal::char_t repl)
{
    int pos = 0;
    while ((pos = path->find(match, pos)) != pal::string_t::npos)
    {
        (*path)[pos] = repl;
    }
}


pal::string_t get_own_rid()
{
#if defined(COREHOST_RID)
    return COREHOST_RID;
#else
    return _X("win10-x64");
    // TODO: #error "Cannot build the host without knowing host's root RID"
#endif
}
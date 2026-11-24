#ifndef ES_COMMAND_BUFFER_H
#define ES_COMMAND_BUFFER_H

// Source's command-buffer grammar belongs to the engine, not CCommand::Tokenize.
inline bool EsCommandNeedsEngineBuffer(const char *text)
{
    bool quoted = false;
    for (const char *cursor = text; *cursor; ++cursor)
    {
        if (*cursor == '\n')
        {
            // Source splits even quoted newlines. Ordinary trailing EOLs are safe.
            if (quoted)
                return true;
            for (++cursor; *cursor; ++cursor)
            {
                if (*cursor != ' ' && *cursor != '\t' && *cursor != '\r' &&
                    *cursor != '\n' && *cursor != '\v' && *cursor != '\f')
                    return true;
            }
            return false;
        }
        if (*cursor == '"')
            quoted = !quoted;
        else if (!quoted && (*cursor == ';' || (*cursor == '/' && cursor[1] == '/')))
            return true;
    }
    return false;
}

#endif

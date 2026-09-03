#include "TreeState.h"

#include <algorithm>

auto TreeStateHelper::SplitPath(const std::wstring& path) -> std::vector<std::wstring>
{
    std::vector<std::wstring> keys;

    if (path.empty())
        return keys;

    size_t start = 0;
    while (start <= path.size())
    {
        auto pos = path.find(L'.', start);
        if (pos == std::wstring::npos)
        {
            keys.emplace_back(path.substr(start));
            break;
        }

        keys.emplace_back(path.substr(start, pos - start));
        start = pos + 1;
    }

    // An empty trailing key (e.g. path ending with a dot) is dropped
    if (!keys.empty() && keys.back().empty())
        keys.pop_back();

    return keys;
}

auto TreeStateHelper::JoinPath(const std::vector<std::wstring>& parentKeys, const std::wstring& key) -> std::wstring
{
    std::wstring path;
    for (const auto& part : parentKeys)
    {
        path += part;
        path += L'.';
    }
    path += key;
    return path;
}

auto TreeStateHelper::MatchExpansion(const TreeExpansionState& oldState,
                                     const std::vector<std::wstring>& newPaths)
    -> std::pair<std::vector<std::wstring>, std::vector<std::wstring>>
{
    std::vector<std::wstring> pathsToExpand;

    for (const auto& path : newPaths)
    {
        auto find = oldState.expandedPaths.find(path);
        if (find != oldState.expandedPaths.cend() && find->second)
            pathsToExpand.push_back(path);
    }

    std::vector<std::wstring> pathToSelect;
    if (!oldState.selectedPath.empty())
    {
        // Reconstruct the selected path in "joined" form and check existence
        std::wstring joined;
        for (const auto& key : oldState.selectedPath)
        {
            if (!joined.empty())
                joined += L'.';
            joined += key;
        }

        if (std::find(newPaths.cbegin(), newPaths.cend(), joined) != newPaths.cend())
            pathToSelect = oldState.selectedPath;
    }

    return {pathsToExpand, pathToSelect};
}

namespace
{
    bool AreNodesEqual(const TreeStateNode& lhs, const TreeStateNode& rhs)
    {
        if (lhs.text != rhs.text || lhs.expanded != rhs.expanded)
            return false;

        if (lhs.pos.has_value() != rhs.pos.has_value())
            return false;

        if (lhs.pos.has_value())
        {
            if (lhs.pos->nLine != rhs.pos->nLine || lhs.pos->nColumn != rhs.pos->nColumn
                || lhs.pos->nKeyLength != rhs.pos->nKeyLength)
                return false;
        }

        if (lhs.children.size() != rhs.children.size())
            return false;

        for (size_t i = 0; i < lhs.children.size(); ++i)
        {
            if (!AreNodesEqual(lhs.children[i], rhs.children[i]))
                return false;
        }

        return true;
    }
}

auto TreeStateHelper::AreEqual(const TreeState& lhs, const TreeState& rhs) -> bool
{
    if (lhs.selectedPath != rhs.selectedPath)
        return false;

    if (lhs.roots.size() != rhs.roots.size())
        return false;

    for (size_t i = 0; i < lhs.roots.size(); ++i)
    {
        if (!AreNodesEqual(lhs.roots[i], rhs.roots[i]))
            return false;
    }

    return true;
}

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include "JsonNode.h"

/*
 * TreeState captures the visual state of the JSON tree (nodes, expansion
 * state and selection) as a plain in-memory model, independent from any
 * Win32 control. It is used to:
 *   - snapshot a tree per Notepad++ tab (buffer) and restore it when the
 *     tab is activated again (without re-parsing),
 *   - restore expansion/selection state after the tree is rebuilt on
 *     refresh, by matching node paths.
 */

struct TreeStateNode
{
    std::wstring            text;      // Display text of the node (including trailing [n]/{n} counts)
    std::optional<Position> pos;       // Editor position of the key (nullopt when the node has none)
    bool                    expanded = false;
    std::vector<TreeStateNode> children;
};

struct TreeState
{
    std::vector<TreeStateNode> roots;          // Children of the tree root ("JSON")
    std::vector<std::wstring>  selectedPath;   // Path of the selected node (key per level), empty when none
};

/*
 * Set of expanded node paths + selected path, used to re-apply expansion
 * state onto a freshly built tree (refresh scenario).
 */
struct TreeExpansionState
{
    std::unordered_map<std::wstring, bool> expandedPaths;    // path -> was expanded
    std::vector<std::wstring>              selectedPath;     // key-path of the selected node, empty when none
};

class TreeStateHelper
{
public:
    // Split a node path (as returned by TreeViewCtrl::GetNodePath) into its keys.
    // Path format examples: "root.child", "root.[0]", "root.[0].[1].key"
    static auto SplitPath(const std::wstring& path) -> std::vector<std::wstring>;

    // Build the path of a node from the path of its parent and the node key.
    static auto JoinPath(const std::vector<std::wstring>& parentKeys, const std::wstring& key) -> std::wstring;

    /*
     * Compute which paths of `oldState` must be re-expanded onto a new tree
     * whose node paths are listed in `newPaths`, and where the selection
     * should be restored (only if the path still exists).
     * Duplicate keys resolve to the first matching node (accepted trade-off).
     */
    static auto MatchExpansion(const TreeExpansionState& oldState,
                               const std::vector<std::wstring>& newPaths)
        -> std::pair<std::vector<std::wstring> /*pathsToExpand*/, std::vector<std::wstring> /*pathToSelect*/>;

    /*
     * Recursive comparison used by unit tests: verifies that two states have
     * identical structure, texts, expansion flags and selection.
     */
    static auto AreEqual(const TreeState& lhs, const TreeState& rhs) -> bool;
};

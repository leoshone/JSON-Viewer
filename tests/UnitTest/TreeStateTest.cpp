#include <gtest/gtest.h>

#include "TreeState.h"

namespace TreeStateTests
{
    TEST(SplitPath, EmptyPath)
    {
        EXPECT_TRUE(TreeStateHelper::SplitPath(L"").empty());
    }

    TEST(SplitPath, SingleKey)
    {
        auto keys = TreeStateHelper::SplitPath(L"root");
        ASSERT_EQ(keys.size(), 1u);
        EXPECT_EQ(keys[0], L"root");
    }

    TEST(SplitPath, MultipleKeys)
    {
        auto keys = TreeStateHelper::SplitPath(L"root.child.[0].name");
        ASSERT_EQ(keys.size(), 4u);
        EXPECT_EQ(keys[0], L"root");
        EXPECT_EQ(keys[1], L"child");
        EXPECT_EQ(keys[2], L"[0]");
        EXPECT_EQ(keys[3], L"name");
    }

    TEST(SplitPath, TrailingDotDropped)
    {
        auto keys = TreeStateHelper::SplitPath(L"root.child.");
        ASSERT_EQ(keys.size(), 2u);
        EXPECT_EQ(keys[1], L"child");
    }

    TEST(JoinPath, EmptyParents)
    {
        EXPECT_EQ(TreeStateHelper::JoinPath({}, L"root"), L"root");
    }

    TEST(JoinPath, Nested)
    {
        EXPECT_EQ(TreeStateHelper::JoinPath({L"root", L"[0]"}, L"key"), L"root.[0].key");
    }

    TEST(MatchExpansion, EmptyOldState)
    {
        TreeExpansionState oldState;
        auto [toExpand, toSelect] = TreeStateHelper::MatchExpansion(oldState, {L"a", L"b"});
        EXPECT_TRUE(toExpand.empty());
        EXPECT_TRUE(toSelect.empty());
    }

    TEST(MatchExpansion, PathStillExists)
    {
        TreeExpansionState oldState;
        oldState.expandedPaths[L"root.child"] = true;
        oldState.expandedPaths[L"root.gone"]  = false;

        auto [toExpand, toSelect] = TreeStateHelper::MatchExpansion(oldState, {L"root", L"root.child", L"root.other"});
        ASSERT_EQ(toExpand.size(), 1u);
        EXPECT_EQ(toExpand[0], L"root.child");
        EXPECT_TRUE(toSelect.empty());
    }

    TEST(MatchExpansion, CollapsedPathsNotReExpanded)
    {
        TreeExpansionState oldState;
        oldState.expandedPaths[L"root.a"] = false;
        oldState.expandedPaths[L"root.b"] = true;

        auto [toExpand, toSelect] = TreeStateHelper::MatchExpansion(oldState, {L"root.a", L"root.b"});
        ASSERT_EQ(toExpand.size(), 1u);
        EXPECT_EQ(toExpand[0], L"root.b");
    }

    TEST(MatchExpansion, SelectionRestoredWhenExists)
    {
        TreeExpansionState oldState;
        oldState.selectedPath = {L"root", L"child"};

        auto [toExpand, toSelect] = TreeStateHelper::MatchExpansion(oldState, {L"root", L"root.child"});
        ASSERT_EQ(toSelect.size(), 2u);
        EXPECT_EQ(toSelect[0], L"root");
        EXPECT_EQ(toSelect[1], L"child");
    }

    TEST(MatchExpansion, SelectionDroppedWhenMissing)
    {
        TreeExpansionState oldState;
        oldState.selectedPath = {L"root", L"gone"};

        auto [toExpand, toSelect] = TreeStateHelper::MatchExpansion(oldState, {L"root", L"root.here"});
        EXPECT_TRUE(toSelect.empty());
    }

    TEST(AreEqual, IdenticalStates)
    {
        TreeStateNode child;
        child.text = L"a : 1";

        TreeStateNode parent;
        parent.text     = L"obj {1}";
        parent.expanded = true;
        parent.children = {child};

        TreeState a, b;
        a.roots.push_back(parent);
        b.roots.push_back(parent);
        a.selectedPath = {L"obj", L"a"};
        b.selectedPath = {L"obj", L"a"};

        EXPECT_TRUE(TreeStateHelper::AreEqual(a, b));
    }

    TEST(AreEqual, DifferentExpansion)
    {
        TreeState a, b;
        TreeStateNode n;
        n.text     = L"key";
        n.expanded = true;
        a.roots.push_back(n);
        n.expanded = false;
        b.roots.push_back(n);

        EXPECT_FALSE(TreeStateHelper::AreEqual(a, b));
    }

    TEST(AreEqual, DifferentSelection)
    {
        TreeState a, b;
        TreeStateNode n;
        n.text = L"key";
        a.roots.push_back(n);
        b.roots.push_back(n);
        a.selectedPath = {L"key"};
        b.selectedPath = {L"other"};

        EXPECT_FALSE(TreeStateHelper::AreEqual(a, b));
    }

    TEST(AreEqual, DifferentChildrenCount)
    {
        TreeState a, b;
        TreeStateNode parent;
        parent.text = L"obj {2}";
        TreeStateNode c1, c2;
        c1.text       = L"a";
        c2.text       = L"b";
        parent.children = {c1, c2};
        a.roots.push_back(parent);

        parent.children = {c1};
        b.roots.push_back(parent);

        EXPECT_FALSE(TreeStateHelper::AreEqual(a, b));
    }

    TEST(AreEqual, PositionCompared)
    {
        TreeState a, b;
        TreeStateNode n;
        n.text     = L"key";
        n.pos      = Position{3, 5, 4};
        a.roots.push_back(n);

        TreeStateNode m;
        m.text = L"key";
        m.pos  = Position{3, 5, 4};
        b.roots.push_back(m);

        EXPECT_TRUE(TreeStateHelper::AreEqual(a, b));

        m.pos = Position{4, 5, 4};
        b.roots.clear();
        b.roots.push_back(m);
        EXPECT_FALSE(TreeStateHelper::AreEqual(a, b));
    }
}

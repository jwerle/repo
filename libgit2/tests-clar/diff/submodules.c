#include "clar_libgit2.h"
#include "repository.h"
#include "posix.h"
#include "../submodule/submodule_helpers.h"

static git_repository *g_repo = NULL;

void test_diff_submodules__initialize(void)
{
}

void test_diff_submodules__cleanup(void)
{
	cleanup_fixture_submodules();
}

static void check_diff_patches_at_line(
	git_diff_list *diff, const char **expected, const char *file, int line)
{
	const git_diff_delta *delta;
	git_diff_patch *patch = NULL;
	size_t d, num_d = git_diff_num_deltas(diff);
	char *patch_text;

	for (d = 0; d < num_d; ++d, git_diff_patch_free(patch)) {
		cl_git_pass(git_diff_get_patch(&patch, &delta, diff, d));

		if (delta->status == GIT_DELTA_UNMODIFIED) {
			cl_assert_at_line(expected[d] == NULL, file, line);
			continue;
		}

		if (expected[d] && !strcmp(expected[d], "<SKIP>"))
			continue;
		if (expected[d] && !strcmp(expected[d], "<END>")) {
			cl_git_pass(git_diff_patch_to_str(&patch_text, patch));
			cl_assert_at_line(!strcmp(expected[d], "<END>"), file, line);
		}

		cl_git_pass(git_diff_patch_to_str(&patch_text, patch));

		clar__assert_equal_s(expected[d], patch_text, file, line,
			"expected diff did not match actual diff", 1);
		git__free(patch_text);
	}

	cl_assert_at_line(expected[d] && !strcmp(expected[d], "<END>"), file, line);
}

#define check_diff_patches(diff, exp) \
	check_diff_patches_at_line(diff, exp, __FILE__, __LINE__)

void test_diff_submodules__unmodified_submodule(void)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff = NULL;
	static const char *expected[] = {
		"<SKIP>", /* .gitmodules */
		NULL, /* added */
		NULL, /* ignored */
		"diff --git a/modified b/modified\nindex 092bfb9..452216e 100644\n--- a/modified\n+++ b/modified\n@@ -1 +1,2 @@\n-yo\n+changed\n+\n", /* modified */
		NULL, /* testrepo.git */
		NULL, /* unmodified */
		NULL, /* untracked */
		"<END>"
	};

	g_repo = setup_fixture_submodules();

	opts.flags = GIT_DIFF_INCLUDE_IGNORED |
		GIT_DIFF_INCLUDE_UNTRACKED |
		GIT_DIFF_INCLUDE_UNMODIFIED;
	opts.old_prefix = "a"; opts.new_prefix = "b";

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected);
	git_diff_list_free(diff);
}

void test_diff_submodules__dirty_submodule(void)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff = NULL;
	static const char *expected[] = {
		"<SKIP>", /* .gitmodules */
		NULL, /* added */
		NULL, /* ignored */
		"diff --git a/modified b/modified\nindex 092bfb9..452216e 100644\n--- a/modified\n+++ b/modified\n@@ -1 +1,2 @@\n-yo\n+changed\n+\n", /* modified */
		"diff --git a/testrepo b/testrepo\nindex a65fedf..a65fedf 160000\n--- a/testrepo\n+++ b/testrepo\n@@ -1 +1 @@\n-Subproject commit a65fedf39aefe402d3bb6e24df4d4f5fe4547750\n+Subproject commit a65fedf39aefe402d3bb6e24df4d4f5fe4547750-dirty\n", /* testrepo.git */
		NULL, /* unmodified */
		NULL, /* untracked */
		"<END>"
	};

	g_repo = setup_fixture_submodules();

	cl_git_rewritefile("submodules/testrepo/README", "heyheyhey");
	cl_git_mkfile("submodules/testrepo/all_new.txt", "never seen before");

	opts.flags = GIT_DIFF_INCLUDE_IGNORED |
		GIT_DIFF_INCLUDE_UNTRACKED |
		GIT_DIFF_INCLUDE_UNMODIFIED;
	opts.old_prefix = "a"; opts.new_prefix = "b";

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected);
	git_diff_list_free(diff);
}

void test_diff_submodules__dirty_submodule_2(void)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff = NULL, *diff2 = NULL;
	char *smpath = "testrepo";
	static const char *expected_none[] = { "<END>" };
	static const char *expected_dirty[] = {
		"diff --git a/testrepo b/testrepo\nindex a65fedf..a65fedf 160000\n--- a/testrepo\n+++ b/testrepo\n@@ -1 +1 @@\n-Subproject commit a65fedf39aefe402d3bb6e24df4d4f5fe4547750\n+Subproject commit a65fedf39aefe402d3bb6e24df4d4f5fe4547750-dirty\n", /* testrepo.git */
		"<END>"
	};

	g_repo = setup_fixture_submodules();

	cl_git_pass(git_submodule_reload_all(g_repo));

	opts.flags = GIT_DIFF_INCLUDE_UNTRACKED |
		GIT_DIFF_INCLUDE_UNTRACKED_CONTENT |
		GIT_DIFF_RECURSE_UNTRACKED_DIRS |
		GIT_DIFF_DISABLE_PATHSPEC_MATCH;
	opts.old_prefix = "a"; opts.new_prefix = "b";
	opts.pathspec.count = 1;
	opts.pathspec.strings = &smpath;

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_none);
	git_diff_list_free(diff);

	cl_git_rewritefile("submodules/testrepo/README", "heyheyhey");
	cl_git_mkfile("submodules/testrepo/all_new.txt", "never seen before");

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_dirty);

	{
		git_tree *head;

		cl_git_pass(git_repository_head_tree(&head, g_repo));
		cl_git_pass(git_diff_tree_to_index(&diff2, g_repo, head, NULL, &opts));
		cl_git_pass(git_diff_merge(diff, diff2));
		git_diff_list_free(diff2);
		git_tree_free(head);

		check_diff_patches(diff, expected_dirty);
	}

	git_diff_list_free(diff);

	cl_git_pass(git_submodule_reload_all(g_repo));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_dirty);
	git_diff_list_free(diff);
}

void test_diff_submodules__submod2_index_to_wd(void)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff = NULL;
	static const char *expected[] = {
		"<SKIP>", /* .gitmodules */
		"diff --git a/sm_changed_file b/sm_changed_file\nindex 4800958..4800958 160000\n--- a/sm_changed_file\n+++ b/sm_changed_file\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0-dirty\n", /* sm_changed_file */
		"diff --git a/sm_changed_head b/sm_changed_head\nindex 4800958..3d9386c 160000\n--- a/sm_changed_head\n+++ b/sm_changed_head\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247\n", /* sm_changed_head */
		"diff --git a/sm_changed_index b/sm_changed_index\nindex 4800958..4800958 160000\n--- a/sm_changed_index\n+++ b/sm_changed_index\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0-dirty\n", /* sm_changed_index */
		"diff --git a/sm_changed_untracked_file b/sm_changed_untracked_file\nindex 4800958..4800958 160000\n--- a/sm_changed_untracked_file\n+++ b/sm_changed_untracked_file\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0-dirty\n", /* sm_changed_untracked_file */
		"diff --git a/sm_missing_commits b/sm_missing_commits\nindex 4800958..5e49635 160000\n--- a/sm_missing_commits\n+++ b/sm_missing_commits\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 5e4963595a9774b90524d35a807169049de8ccad\n", /* sm_missing_commits */
		"<END>"
	};

	g_repo = setup_fixture_submod2();

	opts.flags = GIT_DIFF_INCLUDE_UNTRACKED;
	opts.old_prefix = "a"; opts.new_prefix = "b";

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected);
	git_diff_list_free(diff);
}

void test_diff_submodules__submod2_head_to_index(void)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_tree *head;
	git_diff_list *diff = NULL;
	static const char *expected[] = {
		"<SKIP>", /* .gitmodules */
		"diff --git a/sm_added_and_uncommited b/sm_added_and_uncommited\nnew file mode 160000\nindex 0000000..4800958\n--- /dev/null\n+++ b/sm_added_and_uncommited\n@@ -0,0 +1 @@\n+Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n", /* sm_added_and_uncommited */
		"<END>"
	};

	g_repo = setup_fixture_submod2();

	cl_git_pass(git_repository_head_tree(&head, g_repo));

	opts.flags = GIT_DIFF_INCLUDE_UNTRACKED;
	opts.old_prefix = "a"; opts.new_prefix = "b";

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, head, NULL, &opts));
	check_diff_patches(diff, expected);
	git_diff_list_free(diff);

	git_tree_free(head);
}

void test_diff_submodules__invalid_cache(void)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff = NULL;
	git_submodule *sm;
	char *smpath = "sm_changed_head";
	git_repository *smrepo;
	git_index *smindex;
	static const char *expected_baseline[] = {
		"diff --git a/sm_changed_head b/sm_changed_head\nindex 4800958..3d9386c 160000\n--- a/sm_changed_head\n+++ b/sm_changed_head\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247\n", /* sm_changed_head */
		"<END>"
	};
	static const char *expected_unchanged[] = { "<END>" };
	static const char *expected_dirty[] = {
		"diff --git a/sm_changed_head b/sm_changed_head\nindex 3d9386c..3d9386c 160000\n--- a/sm_changed_head\n+++ b/sm_changed_head\n@@ -1 +1 @@\n-Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247\n+Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247-dirty\n",
		"<END>"
	};
	static const char *expected_moved[] = {
		"diff --git a/sm_changed_head b/sm_changed_head\nindex 3d9386c..0910a13 160000\n--- a/sm_changed_head\n+++ b/sm_changed_head\n@@ -1 +1 @@\n-Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247\n+Subproject commit 0910a13dfa2210496f6c590d75bc360dd11b2a1b\n",
		"<END>"
	};
	static const char *expected_moved_dirty[] = {
		"diff --git a/sm_changed_head b/sm_changed_head\nindex 3d9386c..0910a13 160000\n--- a/sm_changed_head\n+++ b/sm_changed_head\n@@ -1 +1 @@\n-Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247\n+Subproject commit 0910a13dfa2210496f6c590d75bc360dd11b2a1b-dirty\n",
		"<END>"
	};

	g_repo = setup_fixture_submod2();

	opts.flags = GIT_DIFF_INCLUDE_UNTRACKED;
	opts.old_prefix = "a"; opts.new_prefix = "b";
	opts.pathspec.count = 1;
	opts.pathspec.strings = &smpath;

	/* baseline */
	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_baseline);
	git_diff_list_free(diff);

	/* update index with new HEAD */
	cl_git_pass(git_submodule_lookup(&sm, g_repo, smpath));
	cl_git_pass(git_submodule_add_to_index(sm, 1));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_unchanged);
	git_diff_list_free(diff);

	/* create untracked file in submodule working directory */
	cl_git_mkfile("submod2/sm_changed_head/new_around_here", "hello");
	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_NONE);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_dirty);
	git_diff_list_free(diff);

	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_UNTRACKED);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_unchanged);
	git_diff_list_free(diff);

	/* modify tracked file in submodule working directory */
	cl_git_append2file(
		"submod2/sm_changed_head/file_to_modify", "\nmore stuff\n");

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_dirty);
	git_diff_list_free(diff);

	cl_git_pass(git_submodule_reload_all(g_repo));
	cl_git_pass(git_submodule_lookup(&sm, g_repo, smpath));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_dirty);
	git_diff_list_free(diff);

	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_DIRTY);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_unchanged);
	git_diff_list_free(diff);

	/* add file to index in submodule */
	cl_git_pass(git_submodule_open(&smrepo, sm));
	cl_git_pass(git_repository_index(&smindex, smrepo));
	cl_git_pass(git_index_add_bypath(smindex, "file_to_modify"));

	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_UNTRACKED);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_dirty);
	git_diff_list_free(diff);

	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_DIRTY);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_unchanged);
	git_diff_list_free(diff);

	/* commit changed index of submodule */
	{
		git_object *parent;
		git_oid tree_id, commit_id;
		git_tree *tree;
		git_signature *sig;
		git_reference *ref;

		cl_git_pass(git_revparse_ext(&parent, &ref, smrepo, "HEAD"));
		cl_git_pass(git_index_write_tree(&tree_id, smindex));
		cl_git_pass(git_index_write(smindex));
		cl_git_pass(git_tree_lookup(&tree, smrepo, &tree_id));
		cl_git_pass(git_signature_new(&sig, "Sm Test", "sm@tester.test", 1372350000, 480));
		cl_git_pass(git_commit_create_v(
			&commit_id, smrepo, git_reference_name(ref), sig, sig,
			NULL, "Move it", tree, 1, parent));
		git_object_free(parent);
		git_tree_free(tree);
		git_reference_free(ref);
		git_signature_free(sig);
	}

	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_DIRTY);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_moved);
	git_diff_list_free(diff);

	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_ALL);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_unchanged);
	git_diff_list_free(diff);

	git_submodule_set_ignore(sm, GIT_SUBMODULE_IGNORE_NONE);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_moved_dirty);
	git_diff_list_free(diff);

	p_unlink("submod2/sm_changed_head/new_around_here");

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_moved);
	git_diff_list_free(diff);

	git_index_free(smindex);
	git_repository_free(smrepo);
}

void test_diff_submodules__diff_ignore_options(void)
{
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff = NULL;
	git_config *cfg;
	static const char *expected_normal[] = {
		"<SKIP>", /* .gitmodules */
		"diff --git a/sm_changed_file b/sm_changed_file\nindex 4800958..4800958 160000\n--- a/sm_changed_file\n+++ b/sm_changed_file\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0-dirty\n", /* sm_changed_file */
		"diff --git a/sm_changed_head b/sm_changed_head\nindex 4800958..3d9386c 160000\n--- a/sm_changed_head\n+++ b/sm_changed_head\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247\n", /* sm_changed_head */
		"diff --git a/sm_changed_index b/sm_changed_index\nindex 4800958..4800958 160000\n--- a/sm_changed_index\n+++ b/sm_changed_index\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0-dirty\n", /* sm_changed_index */
		"diff --git a/sm_changed_untracked_file b/sm_changed_untracked_file\nindex 4800958..4800958 160000\n--- a/sm_changed_untracked_file\n+++ b/sm_changed_untracked_file\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0-dirty\n", /* sm_changed_untracked_file */
		"diff --git a/sm_missing_commits b/sm_missing_commits\nindex 4800958..5e49635 160000\n--- a/sm_missing_commits\n+++ b/sm_missing_commits\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 5e4963595a9774b90524d35a807169049de8ccad\n", /* sm_missing_commits */
		"<END>"
	};
	static const char *expected_ignore_all[] = {
		"<SKIP>", /* .gitmodules */
		"<END>"
	};
	static const char *expected_ignore_dirty[] = {
		"<SKIP>", /* .gitmodules */
		"diff --git a/sm_changed_head b/sm_changed_head\nindex 4800958..3d9386c 160000\n--- a/sm_changed_head\n+++ b/sm_changed_head\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 3d9386c507f6b093471a3e324085657a3c2b4247\n", /* sm_changed_head */
		"diff --git a/sm_missing_commits b/sm_missing_commits\nindex 4800958..5e49635 160000\n--- a/sm_missing_commits\n+++ b/sm_missing_commits\n@@ -1 +1 @@\n-Subproject commit 480095882d281ed676fe5b863569520e54a7d5c0\n+Subproject commit 5e4963595a9774b90524d35a807169049de8ccad\n", /* sm_missing_commits */
		"<END>"
	};

	g_repo = setup_fixture_submod2();

	opts.flags = GIT_DIFF_INCLUDE_UNTRACKED;
	opts.old_prefix = "a"; opts.new_prefix = "b";

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_normal);
	git_diff_list_free(diff);

	opts.flags |= GIT_DIFF_IGNORE_SUBMODULES;

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_ignore_all);
	git_diff_list_free(diff);

	opts.flags &= ~GIT_DIFF_IGNORE_SUBMODULES;
	opts.ignore_submodules = GIT_SUBMODULE_IGNORE_ALL;

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_ignore_all);
	git_diff_list_free(diff);

	opts.ignore_submodules = GIT_SUBMODULE_IGNORE_DIRTY;

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_ignore_dirty);
	git_diff_list_free(diff);

	opts.ignore_submodules = 0;
	cl_git_pass(git_repository_config(&cfg, g_repo));
	cl_git_pass(git_config_set_bool(cfg, "diff.ignoreSubmodules", false));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_normal);
	git_diff_list_free(diff);

	cl_git_pass(git_config_set_bool(cfg, "diff.ignoreSubmodules", true));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_ignore_all);
	git_diff_list_free(diff);

	cl_git_pass(git_config_set_string(cfg, "diff.ignoreSubmodules", "none"));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_normal);
	git_diff_list_free(diff);

	cl_git_pass(git_config_set_string(cfg, "diff.ignoreSubmodules", "dirty"));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));
	check_diff_patches(diff, expected_ignore_dirty);
	git_diff_list_free(diff);

	git_config_free(cfg);
}

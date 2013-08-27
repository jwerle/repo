#include "clar_libgit2.h"
#include "git2/sys/repository.h"

#include "diff_helpers.h"
#include "repository.h"
#include "buf_text.h"

static git_repository *g_repo = NULL;

void test_diff_patch__initialize(void)
{
}

void test_diff_patch__cleanup(void)
{
	cl_git_sandbox_cleanup();
}

#define EXPECTED_HEADER "diff --git a/subdir.txt b/subdir.txt\n" \
	"deleted file mode 100644\n" \
	"index e8ee89e..0000000\n" \
	"--- a/subdir.txt\n" \
	"+++ /dev/null\n"

#define EXPECTED_HUNK "@@ -1,2 +0,0 @@\n"

static int check_removal_cb(
	const git_diff_delta *delta,
	const git_diff_range *range,
	char line_origin,
	const char *formatted_output,
	size_t output_len,
	void *payload)
{
	GIT_UNUSED(payload);
	GIT_UNUSED(output_len);

	switch (line_origin) {
	case GIT_DIFF_LINE_FILE_HDR:
		cl_assert_equal_s(EXPECTED_HEADER, formatted_output);
		cl_assert(range == NULL);
		goto check_delta;

	case GIT_DIFF_LINE_HUNK_HDR:
		cl_assert_equal_s(EXPECTED_HUNK, formatted_output);
		/* Fall through */

	case GIT_DIFF_LINE_CONTEXT:
	case GIT_DIFF_LINE_DELETION:
		goto check_range;

	default:
		/* unexpected code path */
		return -1;
	}

check_range:
	cl_assert(range != NULL);
	cl_assert_equal_i(1, range->old_start);
	cl_assert_equal_i(2, range->old_lines);
	cl_assert_equal_i(0, range->new_start);
	cl_assert_equal_i(0, range->new_lines);

check_delta:
	cl_assert_equal_s("subdir.txt", delta->old_file.path);
	cl_assert_equal_s("subdir.txt", delta->new_file.path);
	cl_assert_equal_i(GIT_DELTA_DELETED, delta->status);

	return 0;
}

void test_diff_patch__can_properly_display_the_removal_of_a_file(void)
{
	/*
	* $ git diff 26a125e..735b6a2
	* diff --git a/subdir.txt b/subdir.txt
	* deleted file mode 100644
	* index e8ee89e..0000000
	* --- a/subdir.txt
	* +++ /dev/null
	* @@ -1,2 +0,0 @@
	* -Is it a bird?
	* -Is it a plane?
	*/

	const char *one_sha = "26a125e";
	const char *another_sha = "735b6a2";
	git_tree *one, *another;
	git_diff_list *diff;

	g_repo = cl_git_sandbox_init("status");

	one = resolve_commit_oid_to_tree(g_repo, one_sha);
	another = resolve_commit_oid_to_tree(g_repo, another_sha);

	cl_git_pass(git_diff_tree_to_tree(&diff, g_repo, one, another, NULL));

	cl_git_pass(git_diff_print_patch(diff, check_removal_cb, NULL));

	git_diff_list_free(diff);

	git_tree_free(another);
	git_tree_free(one);
}

void test_diff_patch__to_string(void)
{
	const char *one_sha = "26a125e";
	const char *another_sha = "735b6a2";
	git_tree *one, *another;
	git_diff_list *diff;
	git_diff_patch *patch;
	char *text;
	const char *expected = "diff --git a/subdir.txt b/subdir.txt\ndeleted file mode 100644\nindex e8ee89e..0000000\n--- a/subdir.txt\n+++ /dev/null\n@@ -1,2 +0,0 @@\n-Is it a bird?\n-Is it a plane?\n";

	g_repo = cl_git_sandbox_init("status");

	one = resolve_commit_oid_to_tree(g_repo, one_sha);
	another = resolve_commit_oid_to_tree(g_repo, another_sha);

	cl_git_pass(git_diff_tree_to_tree(&diff, g_repo, one, another, NULL));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));

	cl_git_pass(git_diff_get_patch(&patch, NULL, diff, 0));

	cl_git_pass(git_diff_patch_to_str(&text, patch));

	cl_assert_equal_s(expected, text);

	cl_assert_equal_sz(31, git_diff_patch_size(patch, 0, 0, 0));
	cl_assert_equal_sz(31, git_diff_patch_size(patch, 1, 0, 0));
	cl_assert_equal_sz(31 + 16, git_diff_patch_size(patch, 1, 1, 0));
	cl_assert_equal_sz(strlen(expected), git_diff_patch_size(patch, 1, 1, 1));

	git__free(text);
	git_diff_patch_free(patch);
	git_diff_list_free(diff);
	git_tree_free(another);
	git_tree_free(one);
}

void test_diff_patch__config_options(void)
{
	const char *one_sha = "26a125e"; /* current HEAD */
	git_tree *one;
	git_config *cfg;
	git_diff_list *diff;
	git_diff_patch *patch;
	char *text;
	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	char *onefile = "staged_changes_modified_file";
	const char *expected1 = "diff --git c/staged_changes_modified_file i/staged_changes_modified_file\nindex 70bd944..906ee77 100644\n--- c/staged_changes_modified_file\n+++ i/staged_changes_modified_file\n@@ -1 +1,2 @@\n staged_changes_modified_file\n+staged_changes_modified_file\n";
	const char *expected2 = "diff --git i/staged_changes_modified_file w/staged_changes_modified_file\nindex 906ee77..011c344 100644\n--- i/staged_changes_modified_file\n+++ w/staged_changes_modified_file\n@@ -1,2 +1,3 @@\n staged_changes_modified_file\n staged_changes_modified_file\n+staged_changes_modified_file\n";
	const char *expected3 = "diff --git staged_changes_modified_file staged_changes_modified_file\nindex 906ee77..011c344 100644\n--- staged_changes_modified_file\n+++ staged_changes_modified_file\n@@ -1,2 +1,3 @@\n staged_changes_modified_file\n staged_changes_modified_file\n+staged_changes_modified_file\n";
	const char *expected4 = "diff --git staged_changes_modified_file staged_changes_modified_file\nindex 70bd9443ada0..906ee7711f4f 100644\n--- staged_changes_modified_file\n+++ staged_changes_modified_file\n@@ -1 +1,2 @@\n staged_changes_modified_file\n+staged_changes_modified_file\n";

	g_repo = cl_git_sandbox_init("status");
	cl_git_pass(git_repository_config(&cfg, g_repo));
	one = resolve_commit_oid_to_tree(g_repo, one_sha);
	opts.pathspec.count = 1;
	opts.pathspec.strings = &onefile;


	cl_git_pass(git_config_set_string(cfg, "diff.mnemonicprefix", "true"));

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, one, NULL, &opts));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));
	cl_git_pass(git_diff_get_patch(&patch, NULL, diff, 0));
	cl_git_pass(git_diff_patch_to_str(&text, patch));
	cl_assert_equal_s(expected1, text);

	git__free(text);
	git_diff_patch_free(patch);
	git_diff_list_free(diff);

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));
	cl_git_pass(git_diff_get_patch(&patch, NULL, diff, 0));
	cl_git_pass(git_diff_patch_to_str(&text, patch));
	cl_assert_equal_s(expected2, text);

	git__free(text);
	git_diff_patch_free(patch);
	git_diff_list_free(diff);


	cl_git_pass(git_config_set_string(cfg, "diff.noprefix", "true"));

	cl_git_pass(git_diff_index_to_workdir(&diff, g_repo, NULL, &opts));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));
	cl_git_pass(git_diff_get_patch(&patch, NULL, diff, 0));
	cl_git_pass(git_diff_patch_to_str(&text, patch));
	cl_assert_equal_s(expected3, text);

	git__free(text);
	git_diff_patch_free(patch);
	git_diff_list_free(diff);


	cl_git_pass(git_config_set_int32(cfg, "core.abbrev", 12));

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, one, NULL, &opts));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));
	cl_git_pass(git_diff_get_patch(&patch, NULL, diff, 0));
	cl_git_pass(git_diff_patch_to_str(&text, patch));
	cl_assert_equal_s(expected4, text);

	git__free(text);
	git_diff_patch_free(patch);
	git_diff_list_free(diff);

	git_tree_free(one);
	git_config_free(cfg);
}

void test_diff_patch__hunks_have_correct_line_numbers(void)
{
	git_config *cfg;
	git_tree *head;
	git_diff_options opt = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff;
	git_diff_patch *patch;
	const git_diff_delta *delta;
	const git_diff_range *range;
	const char *hdr, *text;
	size_t hdrlen, hunklen, textlen;
	char origin;
	int oldno, newno;
	git_buf old_content = GIT_BUF_INIT, actual = GIT_BUF_INIT;
	const char *new_content = "The Song of Seven Cities\n------------------------\n\nI WAS Lord of Cities very sumptuously builded.\nSeven roaring Cities paid me tribute from afar.\nIvory their outposts were--the guardrooms of them gilded,\nAnd garrisoned with Amazons invincible in war.\n\nThis is some new text;\nNot as good as the old text;\nBut here it is.\n\nSo they warred and trafficked only yesterday, my Cities.\nTo-day there is no mark or mound of where my Cities stood.\nFor the River rose at midnight and it washed away my Cities.\nThey are evened with Atlantis and the towns before the Flood.\n\nRain on rain-gorged channels raised the water-levels round them,\nFreshet backed on freshet swelled and swept their world from sight,\nTill the emboldened floods linked arms and, flashing forward, drowned them--\nDrowned my Seven Cities and their peoples in one night!\n\nLow among the alders lie their derelict foundations,\nThe beams wherein they trusted and the plinths whereon they built--\nMy rulers and their treasure and their unborn populations,\nDead, destroyed, aborted, and defiled with mud and silt!\n\nAnother replacement;\nBreaking up the poem;\nGenerating some hunks.\n\nTo the sound of trumpets shall their seed restore my Cities\nWealthy and well-weaponed, that once more may I behold\nAll the world go softly when it walks before my Cities,\nAnd the horses and the chariots fleeing from them as of old!\n\n  -- Rudyard Kipling\n";

	g_repo = cl_git_sandbox_init("renames");

	cl_git_pass(git_config_new(&cfg));
	git_repository_set_config(g_repo, cfg);

	cl_git_pass(
		git_futils_readbuffer(&old_content, "renames/songof7cities.txt"));

	cl_git_rewritefile("renames/songof7cities.txt", new_content);

	cl_git_pass(git_repository_head_tree(&head, g_repo));

	cl_git_pass(git_diff_tree_to_workdir(&diff, g_repo, head, &opt));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));

	cl_git_pass(git_diff_get_patch(&patch, &delta, diff, 0));

	cl_assert_equal_i(GIT_DELTA_MODIFIED, (int)delta->status);
	cl_assert_equal_i(2, (int)git_diff_patch_num_hunks(patch));

	/* check hunk 0 */

	cl_git_pass(
		git_diff_patch_get_hunk(&range, &hdr, &hdrlen, &hunklen, patch, 0));

	cl_assert_equal_i(18, (int)hunklen);

	cl_assert_equal_i(6, (int)range->old_start);
	cl_assert_equal_i(15, (int)range->old_lines);
	cl_assert_equal_i(6, (int)range->new_start);
	cl_assert_equal_i(9, (int)range->new_lines);

	cl_assert_equal_i(18, (int)git_diff_patch_num_lines_in_hunk(patch, 0));

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 0));
	cl_assert_equal_i(GIT_DIFF_LINE_CONTEXT, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("Ivory their outposts were--the guardrooms of them gilded,\n", actual.ptr);
	cl_assert_equal_i(6, oldno);
	cl_assert_equal_i(6, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 3));
	cl_assert_equal_i(GIT_DIFF_LINE_DELETION, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("All the world went softly when it walked before my Cities--\n", actual.ptr);
	cl_assert_equal_i(9, oldno);
	cl_assert_equal_i(-1, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 12));
	cl_assert_equal_i(GIT_DIFF_LINE_ADDITION, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("This is some new text;\n", actual.ptr);
	cl_assert_equal_i(-1, oldno);
	cl_assert_equal_i(9, newno);

	/* check hunk 1 */

	cl_git_pass(
		git_diff_patch_get_hunk(&range, &hdr, &hdrlen, &hunklen, patch, 1));

	cl_assert_equal_i(18, (int)hunklen);

	cl_assert_equal_i(31, (int)range->old_start);
	cl_assert_equal_i(15, (int)range->old_lines);
	cl_assert_equal_i(25, (int)range->new_start);
	cl_assert_equal_i(9, (int)range->new_lines);

	cl_assert_equal_i(18, (int)git_diff_patch_num_lines_in_hunk(patch, 1));

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 1, 0));
	cl_assert_equal_i(GIT_DIFF_LINE_CONTEXT, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("My rulers and their treasure and their unborn populations,\n", actual.ptr);
	cl_assert_equal_i(31, oldno);
	cl_assert_equal_i(25, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 1, 3));
	cl_assert_equal_i(GIT_DIFF_LINE_DELETION, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("The Daughters of the Palace whom they cherished in my Cities,\n", actual.ptr);
	cl_assert_equal_i(34, oldno);
	cl_assert_equal_i(-1, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 1, 12));
	cl_assert_equal_i(GIT_DIFF_LINE_ADDITION, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("Another replacement;\n", actual.ptr);
	cl_assert_equal_i(-1, oldno);
	cl_assert_equal_i(28, newno);

	git_diff_patch_free(patch);
	git_diff_list_free(diff);

	/* Let's check line numbers when there is no newline */

	git_buf_rtrim(&old_content);
	cl_git_rewritefile("renames/songof7cities.txt", old_content.ptr);

	cl_git_pass(git_diff_tree_to_workdir(&diff, g_repo, head, &opt));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));

	cl_git_pass(git_diff_get_patch(&patch, &delta, diff, 0));

	cl_assert_equal_i(GIT_DELTA_MODIFIED, (int)delta->status);
	cl_assert_equal_i(1, (int)git_diff_patch_num_hunks(patch));

	/* check hunk 0 */

	cl_git_pass(
		git_diff_patch_get_hunk(&range, &hdr, &hdrlen, &hunklen, patch, 0));

	cl_assert_equal_i(6, (int)hunklen);

	cl_assert_equal_i(46, (int)range->old_start);
	cl_assert_equal_i(4, (int)range->old_lines);
	cl_assert_equal_i(46, (int)range->new_start);
	cl_assert_equal_i(4, (int)range->new_lines);

	cl_assert_equal_i(6, (int)git_diff_patch_num_lines_in_hunk(patch, 0));

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 1));
	cl_assert_equal_i(GIT_DIFF_LINE_CONTEXT, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("And the horses and the chariots fleeing from them as of old!\n", actual.ptr);
	cl_assert_equal_i(47, oldno);
	cl_assert_equal_i(47, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 2));
	cl_assert_equal_i(GIT_DIFF_LINE_CONTEXT, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("\n", actual.ptr);
	cl_assert_equal_i(48, oldno);
	cl_assert_equal_i(48, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 3));
	cl_assert_equal_i(GIT_DIFF_LINE_DELETION, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("  -- Rudyard Kipling\n", actual.ptr);
	cl_assert_equal_i(49, oldno);
	cl_assert_equal_i(-1, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 4));
	cl_assert_equal_i(GIT_DIFF_LINE_ADDITION, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("  -- Rudyard Kipling", actual.ptr);
	cl_assert_equal_i(-1, oldno);
	cl_assert_equal_i(49, newno);

	cl_git_pass(git_diff_patch_get_line_in_hunk(
		&origin, &text, &textlen, &oldno, &newno, patch, 0, 5));
	cl_assert_equal_i(GIT_DIFF_LINE_DEL_EOFNL, (int)origin);
	cl_git_pass(git_buf_set(&actual, text, textlen));
	cl_assert_equal_s("\n\\ No newline at end of file\n", actual.ptr);
	cl_assert_equal_i(-1, oldno);
	cl_assert_equal_i(49, newno);

	git_diff_patch_free(patch);
	git_diff_list_free(diff);

	git_buf_free(&actual);
	git_buf_free(&old_content);
	git_tree_free(head);
	git_config_free(cfg);
}

static void check_single_patch_stats(
	git_repository *repo, size_t hunks,
	size_t adds, size_t dels, size_t ctxt, size_t *sizes,
	const char *expected)
{
	git_diff_list *diff;
	git_diff_patch *patch;
	const git_diff_delta *delta;
	size_t actual_ctxt, actual_adds, actual_dels;

	cl_git_pass(git_diff_index_to_workdir(&diff, repo, NULL, NULL));

	cl_assert_equal_i(1, (int)git_diff_num_deltas(diff));

	cl_git_pass(git_diff_get_patch(&patch, &delta, diff, 0));
	cl_assert_equal_i(GIT_DELTA_MODIFIED, (int)delta->status);

	cl_assert_equal_i((int)hunks, (int)git_diff_patch_num_hunks(patch));

	cl_git_pass( git_diff_patch_line_stats(
		&actual_ctxt, &actual_adds, &actual_dels, patch) );

	cl_assert_equal_sz(ctxt, actual_ctxt);
	cl_assert_equal_sz(adds, actual_adds);
	cl_assert_equal_sz(dels, actual_dels);

	if (expected != NULL) {
		char *text;
		cl_git_pass(git_diff_patch_to_str(&text, patch));
		cl_assert_equal_s(expected, text);
		git__free(text);

		cl_assert_equal_sz(
			strlen(expected), git_diff_patch_size(patch, 1, 1, 1));
	}

	if (sizes) {
		if (sizes[0])
			cl_assert_equal_sz(sizes[0], git_diff_patch_size(patch, 0, 0, 0));
		if (sizes[1])
			cl_assert_equal_sz(sizes[1], git_diff_patch_size(patch, 1, 0, 0));
		if (sizes[2])
			cl_assert_equal_sz(sizes[2], git_diff_patch_size(patch, 1, 1, 0));
	}

	/* walk lines in hunk with basic sanity checks */
	for (; hunks > 0; --hunks) {
		size_t i, max_i;
		int lastoldno = -1, oldno, lastnewno = -1, newno;
		char origin;

		max_i = git_diff_patch_num_lines_in_hunk(patch, hunks - 1);

		for (i = 0; i < max_i; ++i) {
			int expected = 1;

			cl_git_pass(git_diff_patch_get_line_in_hunk(
				&origin, NULL, NULL, &oldno, &newno, patch, hunks - 1, i));

			if (origin == GIT_DIFF_LINE_ADD_EOFNL ||
				origin == GIT_DIFF_LINE_DEL_EOFNL ||
				origin == GIT_DIFF_LINE_CONTEXT_EOFNL)
				expected = 0;

			if (oldno >= 0) {
				if (lastoldno >= 0)
					cl_assert_equal_i(expected, oldno - lastoldno);
				lastoldno = oldno;
			}
			if (newno >= 0) {
				if (lastnewno >= 0)
					cl_assert_equal_i(expected, newno - lastnewno);
				lastnewno = newno;
			}
		}
	}

	git_diff_patch_free(patch);
	git_diff_list_free(diff);
}

void test_diff_patch__line_counts_with_eofnl(void)
{
	git_config *cfg;
	git_buf content = GIT_BUF_INIT;
	const char *end;
	git_index *index;
	const char *expected =
		/* below is pasted output of 'git diff' with fn context removed */
		"diff --git a/songof7cities.txt b/songof7cities.txt\n"
		"index 378a7d9..3d0154e 100644\n"
		"--- a/songof7cities.txt\n"
		"+++ b/songof7cities.txt\n"
		"@@ -42,7 +42,7 @@ With peoples undefeated of the dark, enduring blood.\n"
		" \n"
		" To the sound of trumpets shall their seed restore my Cities\n"
		" Wealthy and well-weaponed, that once more may I behold\n"
		"-All the world go softly when it walks before my Cities,\n"
		"+#All the world go softly when it walks before my Cities,\n"
		" And the horses and the chariots fleeing from them as of old!\n"
		" \n"
		"   -- Rudyard Kipling\n"
		"\\ No newline at end of file\n";
	size_t expected_sizes[3] = { 115, 119 + 115 + 114, 119 + 115 + 114 + 71 };

	g_repo = cl_git_sandbox_init("renames");

	cl_git_pass(git_config_new(&cfg));
	git_repository_set_config(g_repo, cfg);

	cl_git_pass(git_futils_readbuffer(&content, "renames/songof7cities.txt"));

	/* remove first line */

	end = git_buf_cstr(&content) + git_buf_find(&content, '\n') + 1;
	git_buf_consume(&content, end);
	cl_git_rewritefile("renames/songof7cities.txt", content.ptr);

	check_single_patch_stats(g_repo, 1, 0, 1, 3, NULL, NULL);

	/* remove trailing whitespace */

	git_buf_rtrim(&content);
	cl_git_rewritefile("renames/songof7cities.txt", content.ptr);

	check_single_patch_stats(g_repo, 2, 1, 2, 6, NULL, NULL);

	/* add trailing whitespace */

	cl_git_pass(git_repository_index(&index, g_repo));
	cl_git_pass(git_index_add_bypath(index, "songof7cities.txt"));
	cl_git_pass(git_index_write(index));
	git_index_free(index);

	cl_git_pass(git_buf_putc(&content, '\n'));
	cl_git_rewritefile("renames/songof7cities.txt", content.ptr);

	check_single_patch_stats(g_repo, 1, 1, 1, 3, NULL, NULL);

	/* no trailing whitespace as context line */

	{
		/* walk back a couple lines, make space and insert char */
		char *scan = content.ptr + content.size;
		int i;

		for (i = 0; i < 5; ++i) {
			for (--scan; scan > content.ptr && *scan != '\n'; --scan)
				/* seek to prev \n */;
		}
		cl_assert(scan > content.ptr);

		/* overwrite trailing \n with right-shifted content */
		memmove(scan + 1, scan, content.size - (scan - content.ptr) - 1);
		/* insert '#' char into space we created */
		scan[1] = '#';
	}
	cl_git_rewritefile("renames/songof7cities.txt", content.ptr);

	check_single_patch_stats(
		g_repo, 1, 1, 1, 6, expected_sizes, expected);

	git_buf_free(&content);
	git_config_free(cfg);
}

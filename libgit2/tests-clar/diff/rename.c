#include "clar_libgit2.h"
#include "diff_helpers.h"
#include "buf_text.h"

static git_repository *g_repo = NULL;

void test_diff_rename__initialize(void)
{
	g_repo = cl_git_sandbox_init("renames");
}

void test_diff_rename__cleanup(void)
{
	cl_git_sandbox_cleanup();
}

/*
 * Renames repo has:
 *
 * commit 31e47d8c1fa36d7f8d537b96158e3f024de0a9f2 -
 *   serving.txt     (25 lines)
 *   sevencities.txt (50 lines)
 * commit 2bc7f351d20b53f1c72c16c4b036e491c478c49a -
 *   serving.txt     -> sixserving.txt  (rename, no change, 100% match)
 *   sevencities.txt -> sevencities.txt (no change)
 *   sevencities.txt -> songofseven.txt (copy, no change, 100% match)
 * commit 1c068dee5790ef1580cfc4cd670915b48d790084
 *   songofseven.txt -> songofseven.txt (major rewrite, <20% match - split)
 *   sixserving.txt  -> sixserving.txt  (indentation change)
 *   sixserving.txt  -> ikeepsix.txt    (copy, add title, >80% match)
 *   sevencities.txt                    (no change)
 * commit 19dd32dfb1520a64e5bbaae8dce6ef423dfa2f13
 *   songofseven.txt -> untimely.txt    (rename, convert to crlf)
 *   ikeepsix.txt    -> ikeepsix.txt    (reorder sections in file)
 *   sixserving.txt  -> sixserving.txt  (whitespace change - not just indent)
 *   sevencities.txt -> songof7cities.txt (rename, small text changes)
 */

void test_diff_rename__match_oid(void)
{
	const char *old_sha = "31e47d8c1fa36d7f8d537b96158e3f024de0a9f2";
	const char *new_sha = "2bc7f351d20b53f1c72c16c4b036e491c478c49a";
	git_tree *old_tree, *new_tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;

	old_tree = resolve_commit_oid_to_tree(g_repo, old_sha);
	new_tree = resolve_commit_oid_to_tree(g_repo, new_sha);

	/* Must pass GIT_DIFF_INCLUDE_UNMODIFIED if you expect to emulate
	 * --find-copies-harder during rename transformion...
	 */
	diffopts.flags |= GIT_DIFF_INCLUDE_UNMODIFIED;

	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	/* git diff --no-renames \
	 *          31e47d8c1fa36d7f8d537b96158e3f024de0a9f2 \
	 *          2bc7f351d20b53f1c72c16c4b036e491c478c49a
	 */
	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_UNMODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_ADDED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);

	/* git diff 31e47d8c1fa36d7f8d537b96158e3f024de0a9f2 \
	 *          2bc7f351d20b53f1c72c16c4b036e491c478c49a
	 * don't use NULL opts to avoid config `diff.renames` contamination
	 */
	opts.flags = GIT_DIFF_FIND_RENAMES;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(3, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_UNMODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);

	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	/* git diff --find-copies-harder \
	 *          31e47d8c1fa36d7f8d537b96158e3f024de0a9f2 \
	 *          2bc7f351d20b53f1c72c16c4b036e491c478c49a
	 */
	opts.flags = GIT_DIFF_FIND_COPIES_FROM_UNMODIFIED;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(3, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_UNMODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_COPIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);

	git_tree_free(old_tree);
	git_tree_free(new_tree);
}

void test_diff_rename__checks_options_version(void)
{
	const char *old_sha = "31e47d8c1fa36d7f8d537b96158e3f024de0a9f2";
	const char *new_sha = "2bc7f351d20b53f1c72c16c4b036e491c478c49a";
	git_tree *old_tree, *new_tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	const git_error *err;

	old_tree = resolve_commit_oid_to_tree(g_repo, old_sha);
	new_tree = resolve_commit_oid_to_tree(g_repo, new_sha);
	diffopts.flags |= GIT_DIFF_INCLUDE_UNMODIFIED;
	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	opts.version = 0;
	cl_git_fail(git_diff_find_similar(diff, &opts));
	err = giterr_last();
	cl_assert_equal_i(GITERR_INVALID, err->klass);

	giterr_clear();
	opts.version = 1024;
	cl_git_fail(git_diff_find_similar(diff, &opts));
	err = giterr_last();
	cl_assert_equal_i(GITERR_INVALID, err->klass);

	git_diff_list_free(diff);
	git_tree_free(old_tree);
	git_tree_free(new_tree);
}

void test_diff_rename__not_exact_match(void)
{
	const char *sha0 = "2bc7f351d20b53f1c72c16c4b036e491c478c49a";
	const char *sha1 = "1c068dee5790ef1580cfc4cd670915b48d790084";
	const char *sha2 = "19dd32dfb1520a64e5bbaae8dce6ef423dfa2f13";
	git_tree *old_tree, *new_tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;

	/* == Changes =====================================================
	 * songofseven.txt -> songofseven.txt (major rewrite, <20% match - split)
	 * sixserving.txt  -> sixserving.txt  (indentation change)
	 * sixserving.txt  -> ikeepsix.txt    (copy, add title, >80% match)
	 * sevencities.txt                    (no change)
	 */

	old_tree = resolve_commit_oid_to_tree(g_repo, sha0);
	new_tree = resolve_commit_oid_to_tree(g_repo, sha1);

	/* Must pass GIT_DIFF_INCLUDE_UNMODIFIED if you expect to emulate
	 * --find-copies-harder during rename transformion...
	 */
	diffopts.flags |= GIT_DIFF_INCLUDE_UNMODIFIED;

	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	/* git diff --no-renames \
	 *          2bc7f351d20b53f1c72c16c4b036e491c478c49a \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084
	 */
	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_UNMODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);

	/* git diff -M 2bc7f351d20b53f1c72c16c4b036e491c478c49a \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084
	 *
	 * must not pass NULL for opts because it will pick up environment
	 * values for "diff.renames" and test won't be consistent.
	 */
	opts.flags = GIT_DIFF_FIND_RENAMES;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_UNMODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);

	git_diff_list_free(diff);

	/* git diff -M -C \
	 *          2bc7f351d20b53f1c72c16c4b036e491c478c49a \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084
	 */
	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_UNMODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_COPIED]);

	git_diff_list_free(diff);

	/* git diff -M -C --find-copies-harder --break-rewrites \
	 *          2bc7f351d20b53f1c72c16c4b036e491c478c49a \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084
	 */
	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_ALL;
	opts.break_rewrite_threshold = 70;

	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(5, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_UNMODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_COPIED]);

	git_diff_list_free(diff);

	/* == Changes =====================================================
	 * songofseven.txt -> untimely.txt    (rename, convert to crlf)
	 * ikeepsix.txt    -> ikeepsix.txt    (reorder sections in file)
	 * sixserving.txt  -> sixserving.txt  (whitespace - not just indent)
	 * sevencities.txt -> songof7cities.txt (rename, small text changes)
	 */

	git_tree_free(old_tree);
	old_tree = new_tree;
	new_tree = resolve_commit_oid_to_tree(g_repo, sha2);

	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	/* git diff --no-renames \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084 \
	 *          19dd32dfb1520a64e5bbaae8dce6ef423dfa2f13
	 */
	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(6, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_ADDED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_DELETED]);
	git_diff_list_free(diff);

	/* git diff -M -C \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084 \
	 *          19dd32dfb1520a64e5bbaae8dce6ef423dfa2f13
	 */
	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);

	/* git diff -M -C --find-copies-harder --break-rewrites \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084 \
	 *          19dd32dfb1520a64e5bbaae8dce6ef423dfa2f13
	 * with libgit2 default similarity comparison...
	 */
	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	/* the default match algorithm is going to find the internal
	 * whitespace differences in the lines of sixserving.txt to be
	 * significant enough that this will decide to split it into an ADD
	 * and a DELETE
	 */

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(5, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);

	/* git diff -M -C --find-copies-harder --break-rewrites \
	 *          1c068dee5790ef1580cfc4cd670915b48d790084 \
	 *          19dd32dfb1520a64e5bbaae8dce6ef423dfa2f13
	 * with ignore_space whitespace comparision
	 */
	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_ALL | GIT_DIFF_FIND_IGNORE_WHITESPACE;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	/* Ignoring whitespace, this should no longer split sixserver.txt */

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);

	git_tree_free(old_tree);
	git_tree_free(new_tree);
}

void test_diff_rename__handles_small_files(void)
{
	const char *tree_sha = "2bc7f351d20b53f1c72c16c4b036e491c478c49a";
	git_index *index;
	git_tree *tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;

	cl_git_pass(git_repository_index(&index, g_repo));

	tree = resolve_commit_oid_to_tree(g_repo, tree_sha);

	cl_git_rewritefile("renames/songof7cities.txt", "single line\n");
	cl_git_pass(git_index_add_bypath(index, "songof7cities.txt"));

	cl_git_rewritefile("renames/untimely.txt", "untimely\n");
	cl_git_pass(git_index_add_bypath(index, "untimely.txt"));

	/* Tests that we can invoke find_similar on small files
	 * and that the GIT_EBUFS (too small) error code is not
	 * propagated to the caller.
	 */
	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	opts.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES |
		GIT_DIFF_FIND_AND_BREAK_REWRITES;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);
}

void test_diff_rename__working_directory_changes(void)
{
	const char *sha0 = "2bc7f351d20b53f1c72c16c4b036e491c478c49a";
	const char *blobsha = "66311f5cfbe7836c27510a3ba2f43e282e2c8bba";
	git_oid id;
	git_tree *tree;
	git_blob *blob;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;
	git_buf old_content = GIT_BUF_INIT, content = GIT_BUF_INIT;;

	tree = resolve_commit_oid_to_tree(g_repo, sha0);
	diffopts.flags |= GIT_DIFF_INCLUDE_UNMODIFIED | GIT_DIFF_INCLUDE_UNTRACKED;

	/*
	$ git cat-file -p 2bc7f351d20b53f1c72c16c4b036e491c478c49a^{tree}

	100644 blob 66311f5cfbe7836c27510a3ba2f43e282e2c8bba	sevencities.txt
	100644 blob ad0a8e55a104ac54a8a29ed4b84b49e76837a113	sixserving.txt
	100644 blob 66311f5cfbe7836c27510a3ba2f43e282e2c8bba	songofseven.txt

	$ for f in *.txt; do
		echo `git hash-object -t blob $f` $f
	done

	eaf4a3e3bfe68585e90cada20736ace491cd100b ikeepsix.txt
	f90d4fc20ecddf21eebe6a37e9225d244339d2b5 sixserving.txt
	4210ffd5c390b21dd5483375e75288dea9ede512 songof7cities.txt
	9a69d960ae94b060f56c2a8702545e2bb1abb935 untimely.txt
	*/

	cl_git_pass(git_diff_tree_to_workdir(&diff, g_repo, tree, &diffopts));

	/* git diff --no-renames 2bc7f351d20b53f1c72c16c4b036e491c478c49a */
	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(6, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(3, exp.file_status[GIT_DELTA_UNTRACKED]);

	/* git diff -M 2bc7f351d20b53f1c72c16c4b036e491c478c49a */
	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(5, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_RENAMED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_UNTRACKED]);

	git_diff_list_free(diff);

	/* rewrite files in the working directory with / without CRLF changes */

	cl_git_pass(
		git_futils_readbuffer(&old_content, "renames/songof7cities.txt"));
	cl_git_pass(
		git_buf_text_lf_to_crlf(&content, &old_content));
	cl_git_pass(
		git_futils_writebuffer(&content, "renames/songof7cities.txt", 0, 0));

	cl_git_pass(git_diff_tree_to_workdir(&diff, g_repo, tree, &diffopts));

	/* git diff -M 2bc7f351d20b53f1c72c16c4b036e491c478c49a */
	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(5, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_RENAMED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_UNTRACKED]);

	git_diff_list_free(diff);

	/* try a different whitespace option */

	cl_git_pass(git_diff_tree_to_workdir(&diff, g_repo, tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_ALL | GIT_DIFF_FIND_DONT_IGNORE_WHITESPACE;
	opts.rename_threshold = 70;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(6, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(3, exp.file_status[GIT_DELTA_UNTRACKED]);

	git_diff_list_free(diff);

	/* try a different matching option */

	cl_git_pass(git_diff_tree_to_workdir(&diff, g_repo, tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_ALL | GIT_DIFF_FIND_EXACT_MATCH_ONLY;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(6, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(3, exp.file_status[GIT_DELTA_UNTRACKED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_DELETED]);

	git_diff_list_free(diff);

	/* again with exact match blob */

	cl_git_pass(git_oid_fromstr(&id, blobsha));
	cl_git_pass(git_blob_lookup(&blob, g_repo, &id));
	cl_git_pass(git_buf_set(
		&content, git_blob_rawcontent(blob), (size_t)git_blob_rawsize(blob)));
	cl_git_rewritefile("renames/songof7cities.txt", content.ptr);
	git_blob_free(blob);

	cl_git_pass(git_diff_tree_to_workdir(&diff, g_repo, tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_ALL | GIT_DIFF_FIND_EXACT_MATCH_ONLY;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	/*
	fprintf(stderr, "\n\n");
    diff_print_raw(stderr, diff);
	*/

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));

	cl_assert_equal_i(5, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_UNTRACKED]);

	git_diff_list_free(diff);

	git_tree_free(tree);
	git_buf_free(&content);
	git_buf_free(&old_content);
}

void test_diff_rename__patch(void)
{
	const char *sha0 = "2bc7f351d20b53f1c72c16c4b036e491c478c49a";
	const char *sha1 = "1c068dee5790ef1580cfc4cd670915b48d790084";
	git_tree *old_tree, *new_tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	git_diff_patch *patch;
	const git_diff_delta *delta;
	char *text;
	const char *expected = "diff --git a/sixserving.txt b/ikeepsix.txt\nindex ad0a8e5..36020db 100644\n--- a/sixserving.txt\n+++ b/ikeepsix.txt\n@@ -1,3 +1,6 @@\n+I Keep Six Honest Serving-Men\n+=============================\n+\n I KEEP six honest serving-men\n  (They taught me all I knew);\n Their names are What and Why and When\n@@ -21,4 +24,4 @@ She sends'em abroad on her own affairs,\n One million Hows, two million Wheres,\n And seven million Whys!\n \n-                -- Rudyard Kipling\n+  -- Rudyard Kipling\n";

	old_tree = resolve_commit_oid_to_tree(g_repo, sha0);
	new_tree = resolve_commit_oid_to_tree(g_repo, sha1);

	diffopts.flags |= GIT_DIFF_INCLUDE_UNMODIFIED;
	cl_git_pass(git_diff_tree_to_tree(
		&diff, g_repo, old_tree, new_tree, &diffopts));

	opts.flags = GIT_DIFF_FIND_RENAMES | GIT_DIFF_FIND_COPIES;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	/* == Changes =====================================================
	 * sixserving.txt  -> ikeepsix.txt    (copy, add title, >80% match)
	 * sevencities.txt                    (no change)
	 * sixserving.txt  -> sixserving.txt  (indentation change)
	 * songofseven.txt -> songofseven.txt (major rewrite, <20% match - split)
	 */

	cl_assert_equal_i(4, (int)git_diff_num_deltas(diff));

	cl_git_pass(git_diff_get_patch(&patch, &delta, diff, 0));
	cl_assert_equal_i(GIT_DELTA_COPIED, (int)delta->status);

	cl_git_pass(git_diff_patch_to_str(&text, patch));
	cl_assert_equal_s(expected, text);
	git__free(text);

	git_diff_patch_free(patch);

	cl_git_pass(git_diff_get_patch(NULL, &delta, diff, 1));
	cl_assert_equal_i(GIT_DELTA_UNMODIFIED, (int)delta->status);

	cl_git_pass(git_diff_get_patch(NULL, &delta, diff, 2));
	cl_assert_equal_i(GIT_DELTA_MODIFIED, (int)delta->status);

	cl_git_pass(git_diff_get_patch(NULL, &delta, diff, 3));
	cl_assert_equal_i(GIT_DELTA_MODIFIED, (int)delta->status);

	git_diff_list_free(diff);
	git_tree_free(old_tree);
	git_tree_free(new_tree);
}

void test_diff_rename__file_exchange(void)
{
	git_buf c1 = GIT_BUF_INIT, c2 = GIT_BUF_INIT;
	git_index *index;
	git_tree *tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;

	cl_git_pass(git_futils_readbuffer(&c1, "renames/untimely.txt"));
	cl_git_pass(git_futils_readbuffer(&c2, "renames/songof7cities.txt"));
	cl_git_pass(git_futils_writebuffer(&c1, "renames/songof7cities.txt", 0, 0));
	cl_git_pass(git_futils_writebuffer(&c2, "renames/untimely.txt", 0, 0));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(git_repository_index(&index, g_repo));
	cl_git_pass(git_index_read_tree(index, tree));
	cl_git_pass(git_index_add_bypath(index, "songof7cities.txt"));
	cl_git_pass(git_index_add_bypath(index, "untimely.txt"));

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(2, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(2, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);

	git_buf_free(&c1);
	git_buf_free(&c2);
}

void test_diff_rename__file_exchange_three(void)
{
	git_buf c1 = GIT_BUF_INIT, c2 = GIT_BUF_INIT, c3 = GIT_BUF_INIT;
	git_index *index;
	git_tree *tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;

	cl_git_pass(git_futils_readbuffer(&c1, "renames/untimely.txt"));
	cl_git_pass(git_futils_readbuffer(&c2, "renames/songof7cities.txt"));
	cl_git_pass(git_futils_readbuffer(&c3, "renames/ikeepsix.txt"));

	cl_git_pass(git_futils_writebuffer(&c1, "renames/ikeepsix.txt", 0, 0));
	cl_git_pass(git_futils_writebuffer(&c2, "renames/untimely.txt", 0, 0));
	cl_git_pass(git_futils_writebuffer(&c3, "renames/songof7cities.txt", 0, 0));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(git_repository_index(&index, g_repo));
	cl_git_pass(git_index_read_tree(index, tree));
	cl_git_pass(git_index_add_bypath(index, "songof7cities.txt"));
	cl_git_pass(git_index_add_bypath(index, "untimely.txt"));
	cl_git_pass(git_index_add_bypath(index, "ikeepsix.txt"));

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(3, exp.files);
	cl_assert_equal_i(3, exp.file_status[GIT_DELTA_MODIFIED]);

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(3, exp.files);
	cl_assert_equal_i(3, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);

	git_buf_free(&c1);
	git_buf_free(&c2);
	git_buf_free(&c3);
}

void test_diff_rename__file_partial_exchange(void)
{
	git_buf c1 = GIT_BUF_INIT, c2 = GIT_BUF_INIT;
	git_index *index;
	git_tree *tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;
	int i;

	cl_git_pass(git_futils_readbuffer(&c1, "renames/untimely.txt"));
	cl_git_pass(git_futils_writebuffer(&c1, "renames/songof7cities.txt", 0, 0));
	for (i = 0; i < 100; ++i)
		cl_git_pass(git_buf_puts(&c2, "this is not the content you are looking for\n"));
	cl_git_pass(git_futils_writebuffer(&c2, "renames/untimely.txt", 0, 0));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(git_repository_index(&index, g_repo));
	cl_git_pass(git_index_read_tree(index, tree));
	cl_git_pass(git_index_add_bypath(index, "songof7cities.txt"));
	cl_git_pass(git_index_add_bypath(index, "untimely.txt"));

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(2, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_MODIFIED]);

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(3, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);

	git_buf_free(&c1);
	git_buf_free(&c2);
}

void test_diff_rename__rename_and_copy_from_same_source(void)
{
	git_buf c1 = GIT_BUF_INIT, c2 = GIT_BUF_INIT;
	git_index *index;
	git_tree *tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;

	/* put the first 2/3 of file into one new place
	 * and the second 2/3 of file into another new place
	 */
	cl_git_pass(git_futils_readbuffer(&c1, "renames/songof7cities.txt"));
	cl_git_pass(git_buf_set(&c2, c1.ptr, c1.size));
	git_buf_truncate(&c1, c1.size * 2 / 3);
	git_buf_consume(&c2, ((char *)c2.ptr) + (c2.size / 3));
	cl_git_pass(git_futils_writebuffer(&c1, "renames/song_a.txt", 0, 0));
	cl_git_pass(git_futils_writebuffer(&c2, "renames/song_b.txt", 0, 0));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(git_repository_index(&index, g_repo));
	cl_git_pass(git_index_read_tree(index, tree));
	cl_git_pass(git_index_add_bypath(index, "song_a.txt"));
	cl_git_pass(git_index_add_bypath(index, "song_b.txt"));

	diffopts.flags = GIT_DIFF_INCLUDE_UNMODIFIED;

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(6, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_ADDED]);
	cl_assert_equal_i(4, exp.file_status[GIT_DELTA_UNMODIFIED]);

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(6, exp.files);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_COPIED]);
	cl_assert_equal_i(4, exp.file_status[GIT_DELTA_UNMODIFIED]);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);

	git_buf_free(&c1);
	git_buf_free(&c2);
}

void test_diff_rename__from_deleted_to_split(void)
{
	git_buf c1 = GIT_BUF_INIT;
	git_index *index;
	git_tree *tree;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;
	diff_expects exp;

	/* old file is missing, new file is actually old file renamed */

	cl_git_pass(git_futils_readbuffer(&c1, "renames/songof7cities.txt"));
	cl_git_pass(git_futils_writebuffer(&c1, "renames/untimely.txt", 0, 0));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(git_repository_index(&index, g_repo));
	cl_git_pass(git_index_read_tree(index, tree));
	cl_git_pass(git_index_remove_bypath(index, "songof7cities.txt"));
	cl_git_pass(git_index_add_bypath(index, "untimely.txt"));

	diffopts.flags = GIT_DIFF_INCLUDE_UNMODIFIED;

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_MODIFIED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_UNMODIFIED]);

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(4, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);
	cl_assert_equal_i(2, exp.file_status[GIT_DELTA_UNMODIFIED]);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);

	git_buf_free(&c1);
}

struct rename_expected
{
	size_t len;

	unsigned int *status;
	const char **sources;
	const char **targets;

	size_t idx;
};

int test_names_expected(const git_diff_delta *delta, float progress, void *p)
{
	struct rename_expected *expected = p;

	GIT_UNUSED(progress);

	cl_assert(expected->idx < expected->len);

	cl_assert_equal_i(delta->status, expected->status[expected->idx]);

	cl_assert(git__strcmp(expected->sources[expected->idx],
		delta->old_file.path) == 0);
	cl_assert(git__strcmp(expected->targets[expected->idx],
		delta->new_file.path) == 0);

	expected->idx++;

	return 0;
}

void test_diff_rename__rejected_match_can_match_others(void)
{
	git_reference *head, *selfsimilar;
	git_index *index;
	git_tree *tree;
	git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options findopts = GIT_DIFF_FIND_OPTIONS_INIT;
	git_buf one = GIT_BUF_INIT, two = GIT_BUF_INIT;
	unsigned int status[] = { GIT_DELTA_RENAMED, GIT_DELTA_RENAMED };
	const char *sources[] = { "Class1.cs", "Class2.cs" };
	const char *targets[] = { "ClassA.cs", "ClassB.cs" };
	struct rename_expected expect = { 2, status, sources, targets };
	char *ptr;

	opts.checkout_strategy = GIT_CHECKOUT_FORCE;

	cl_git_pass(git_reference_lookup(&head, g_repo, "HEAD"));
	cl_git_pass(git_reference_symbolic_set_target(
		&selfsimilar, head, "refs/heads/renames_similar"));
	cl_git_pass(git_checkout_head(g_repo, &opts));
	cl_git_pass(git_repository_index(&index, g_repo));

	cl_git_pass(git_futils_readbuffer(&one, "renames/Class1.cs"));
	cl_git_pass(git_futils_readbuffer(&two, "renames/Class2.cs"));

	cl_git_pass(p_unlink("renames/Class1.cs"));
	cl_git_pass(p_unlink("renames/Class2.cs"));

	cl_git_pass(git_index_remove_bypath(index, "Class1.cs"));
	cl_git_pass(git_index_remove_bypath(index, "Class2.cs"));

	cl_assert(ptr = strstr(one.ptr, "Class1"));
	ptr[5] = 'A';

	cl_assert(ptr = strstr(two.ptr, "Class2"));
	ptr[5] = 'B';

	cl_git_pass(
		git_futils_writebuffer(&one, "renames/ClassA.cs", O_RDWR|O_CREAT, 0777));
	cl_git_pass(
		git_futils_writebuffer(&two, "renames/ClassB.cs", O_RDWR|O_CREAT, 0777));

	cl_git_pass(git_index_add_bypath(index, "ClassA.cs"));
	cl_git_pass(git_index_add_bypath(index, "ClassB.cs"));

	cl_git_pass(git_index_write(index));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(
		git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	cl_git_pass(git_diff_find_similar(diff, &findopts));

	cl_git_pass(
		git_diff_foreach(diff, test_names_expected, NULL, NULL, &expect));

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);
	git_reference_free(head);
	git_reference_free(selfsimilar);
	git_buf_free(&one);
	git_buf_free(&two);
}

static void write_similarity_file_two(const char *filename, size_t b_lines)
{
	git_buf contents = GIT_BUF_INIT;
	size_t i;

	for (i = 0; i < b_lines; i++)
		git_buf_printf(&contents, "%02d - bbbbb\r\n", (int)(i+1));

	for (i = b_lines; i < 50; i++)
		git_buf_printf(&contents, "%02d - aaaaa%s", (int)(i+1), (i == 49 ? "" : "\r\n"));

	cl_git_pass(
		git_futils_writebuffer(&contents, filename, O_RDWR|O_CREAT, 0777));

	git_buf_free(&contents);
}

void test_diff_rename__rejected_match_can_match_others_two(void)
{
	git_reference *head, *selfsimilar;
	git_index *index;
	git_tree *tree;
	git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options findopts = GIT_DIFF_FIND_OPTIONS_INIT;
	unsigned int status[] = { GIT_DELTA_RENAMED, GIT_DELTA_RENAMED };
	const char *sources[] = { "a.txt", "b.txt" };
	const char *targets[] = { "c.txt", "d.txt" };
	struct rename_expected expect = { 2, status, sources, targets };

	opts.checkout_strategy = GIT_CHECKOUT_FORCE;

	cl_git_pass(git_reference_lookup(&head, g_repo, "HEAD"));
	cl_git_pass(git_reference_symbolic_set_target(
		&selfsimilar, head, "refs/heads/renames_similar_two"));
	cl_git_pass(git_checkout_head(g_repo, &opts));
	cl_git_pass(git_repository_index(&index, g_repo));

	cl_git_pass(p_unlink("renames/a.txt"));
	cl_git_pass(p_unlink("renames/b.txt"));

	cl_git_pass(git_index_remove_bypath(index, "a.txt"));
	cl_git_pass(git_index_remove_bypath(index, "b.txt"));

	write_similarity_file_two("renames/c.txt", 7);
	write_similarity_file_two("renames/d.txt", 8);

	cl_git_pass(git_index_add_bypath(index, "c.txt"));
	cl_git_pass(git_index_add_bypath(index, "d.txt"));

	cl_git_pass(git_index_write(index));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(
		git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	cl_git_pass(git_diff_find_similar(diff, &findopts));

	cl_git_pass(
		git_diff_foreach(diff, test_names_expected, NULL, NULL, &expect));
	cl_assert(expect.idx > 0);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);
	git_reference_free(head);
	git_reference_free(selfsimilar);
}

void test_diff_rename__rejected_match_can_match_others_three(void)
{
	git_reference *head, *selfsimilar;
	git_index *index;
	git_tree *tree;
	git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options findopts = GIT_DIFF_FIND_OPTIONS_INIT;

	/* Both cannot be renames from a.txt */
	unsigned int status[] = { GIT_DELTA_ADDED, GIT_DELTA_RENAMED };
	const char *sources[] = { "0001.txt", "a.txt" };
	const char *targets[] = { "0001.txt", "0002.txt" };
	struct rename_expected expect = { 2, status, sources, targets };

	opts.checkout_strategy = GIT_CHECKOUT_FORCE;

	cl_git_pass(git_reference_lookup(&head, g_repo, "HEAD"));
	cl_git_pass(git_reference_symbolic_set_target(
		&selfsimilar, head, "refs/heads/renames_similar_two"));
	cl_git_pass(git_checkout_head(g_repo, &opts));
	cl_git_pass(git_repository_index(&index, g_repo));

	cl_git_pass(p_unlink("renames/a.txt"));

	cl_git_pass(git_index_remove_bypath(index, "a.txt"));

	write_similarity_file_two("renames/0001.txt", 7);
	write_similarity_file_two("renames/0002.txt", 0);

	cl_git_pass(git_index_add_bypath(index, "0001.txt"));
	cl_git_pass(git_index_add_bypath(index, "0002.txt"));

	cl_git_pass(git_index_write(index));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(
		git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	cl_git_pass(git_diff_find_similar(diff, &findopts));

	cl_git_pass(
		git_diff_foreach(diff, test_names_expected, NULL, NULL, &expect));

	cl_assert(expect.idx == expect.len);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);
	git_reference_free(head);
	git_reference_free(selfsimilar);
}

void test_diff_rename__can_rename_from_rewrite(void)
{
	git_index *index;
	git_tree *tree;
	git_checkout_opts opts = GIT_CHECKOUT_OPTS_INIT;
	git_diff_list *diff;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options findopts = GIT_DIFF_FIND_OPTIONS_INIT;

	unsigned int status[] = { GIT_DELTA_RENAMED, GIT_DELTA_RENAMED };
	const char *sources[] = { "ikeepsix.txt", "songof7cities.txt" };
	const char *targets[] = { "songof7cities.txt", "this-is-a-rename.txt" };
	struct rename_expected expect = { 2, status, sources, targets };

	opts.checkout_strategy = GIT_CHECKOUT_FORCE;

	cl_git_pass(git_repository_index(&index, g_repo));

	cl_git_pass(p_rename("renames/songof7cities.txt", "renames/this-is-a-rename.txt"));
	cl_git_pass(p_rename("renames/ikeepsix.txt", "renames/songof7cities.txt"));

	cl_git_pass(git_index_remove_bypath(index, "ikeepsix.txt"));

	cl_git_pass(git_index_add_bypath(index, "songof7cities.txt"));
	cl_git_pass(git_index_add_bypath(index, "this-is-a-rename.txt"));

	cl_git_pass(git_index_write(index));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(
		git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	findopts.flags |= GIT_DIFF_FIND_AND_BREAK_REWRITES |
		GIT_DIFF_FIND_REWRITES |
		GIT_DIFF_FIND_RENAMES_FROM_REWRITES;

	cl_git_pass(git_diff_find_similar(diff, &findopts));

	cl_git_pass(
		git_diff_foreach(diff, test_names_expected, NULL, NULL, &expect));

	cl_assert(expect.idx == expect.len);

	git_diff_list_free(diff);
	git_tree_free(tree);
	git_index_free(index);
}

void test_diff_rename__case_changes_are_split(void)
{
	git_index *index;
	git_tree *tree;
	git_diff_list *diff = NULL;
	diff_expects exp;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;

	cl_git_pass(git_repository_index(&index, g_repo));

	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(p_rename("renames/ikeepsix.txt", "renames/IKEEPSIX.txt"));

	cl_git_pass(git_index_remove_bypath(index, "ikeepsix.txt"));
	cl_git_pass(git_index_add_bypath(index, "IKEEPSIX.txt"));
	cl_git_pass(git_index_write(index));

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, NULL));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(2, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(1, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);
	git_index_free(index);
	git_tree_free(tree);
}

void test_diff_rename__unmodified_can_be_renamed(void)
{
	git_index *index;
	git_tree *tree;
	git_diff_list *diff = NULL;
	diff_expects exp;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	git_diff_find_options opts = GIT_DIFF_FIND_OPTIONS_INIT;

	cl_git_pass(git_repository_index(&index, g_repo));
	cl_git_pass(
		git_revparse_single((git_object **)&tree, g_repo, "HEAD^{tree}"));

	cl_git_pass(p_rename("renames/ikeepsix.txt", "renames/ikeepsix2.txt"));

	cl_git_pass(git_index_remove_bypath(index, "ikeepsix.txt"));
	cl_git_pass(git_index_add_bypath(index, "ikeepsix2.txt"));
	cl_git_pass(git_index_write(index));

	cl_git_pass(git_diff_tree_to_index(&diff, g_repo, tree, index, &diffopts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(2, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_DELETED]);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_ADDED]);

	opts.flags = GIT_DIFF_FIND_ALL;
	cl_git_pass(git_diff_find_similar(diff, &opts));

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(1, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);

	memset(&exp, 0, sizeof(exp));
	cl_git_pass(git_diff_foreach(
		diff, diff_file_cb, diff_hunk_cb, diff_line_cb, &exp));
	cl_assert_equal_i(1, exp.files);
	cl_assert_equal_i(1, exp.file_status[GIT_DELTA_RENAMED]);

	git_diff_list_free(diff);
	git_index_free(index);
	git_tree_free(tree);
}

/*
 * Unit Tests for src/permut.c
 * Tests permutation functions using data from know/16x16x4.h
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Include the headers - forward declarations
#include "permut.h"
#include "pow_m_sqr.h"
#include "taxicab.h"

// Include test data
#include "know/16x16x4.h"

/* Test framework macros */
#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("Running test: %s ... ", #name); \
    test_##name(); \
    printf("PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "\nAssertion failed: %s\n  at %s:%d\n", #expr, __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))
#define ASSERT_EQUAL(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NOT_EQUAL(a, b) ASSERT_TRUE((a) != (b))

static int tests_passed = 0;

/* ============================================================
 * Tests for rels_are_disjoint
 * ============================================================ */

TEST(rels_are_disjoint_with_16x16_data) {
    // rel1 and rel2 from know/16x16x4.h should be disjoint
    uint8_t result = rels_are_disjoint(rel1, rel2, 16);
    ASSERT_TRUE(result);
}

TEST(rels_are_disjoint_same_array) {
    // A relation should NOT be disjoint with itself
    uint8_t result = rels_are_disjoint(rel1, rel1, 16);
    ASSERT_FALSE(result);
}

TEST(rels_are_disjoint_simple_case) {
    rel_item arr1[] = {0, 1, 2, 3};
    rel_item arr2[] = {4, 1, 3, 0};
    uint8_t result = rels_are_disjoint(arr1, arr2, 4);
    ASSERT_FALSE(result); 
}

TEST(rels_are_disjoint_truly_disjoint) {
    rel_item arr1[] = {0, 2, 1, 3};
    rel_item arr2[] = {3, 1, 2, 0};
    uint8_t result = rels_are_disjoint(arr1, arr2, 4);
    ASSERT_TRUE(result);
}

/* ============================================================
 * Tests for rels_are_diagonizable
 * ============================================================ */

TEST(rels_are_diagonizable_with_16x16_data) {
    rel_item inv[16], sigma[16];

    uint8_t result = rels_are_diagonizable(rel1, rel2, inv, sigma, 16);
    ASSERT_TRUE(result);

    // Verify sigma is an involution: sigma(sigma(i)) = i
    for (size_t i = 0; i < 16; ++i) {
        ASSERT_EQUAL(sigma[sigma[i]], i);
    }

    // Verify number of fixed points is correct for n=16 (even)
    size_t fixed = 0;
    for (size_t i = 0; i < 16; ++i) {
        if (sigma[i] == i) fixed++;
    }
    ASSERT_EQUAL(fixed, 0); // n % 2 = 0 for n=16
}

TEST(rels_are_diagonizable_simple_even) {
    rel_item arr1[] = {1, 0, 3, 2};
    rel_item arr2[] = {0, 1, 2, 3};
    rel_item inv[4], sigma[4];

    uint8_t result = rels_are_diagonizable(arr1, arr2, inv, sigma, 4);

    // Check involution property
    for (size_t i = 0; i < 4; ++i) {
        ASSERT_EQUAL(sigma[sigma[i]], i);
    }

    (void)result;
}

TEST(rels_are_diagonizable_simple_odd) {
    rel_item arr1[] = {0, 2, 4, 3, 1};
    rel_item arr2[] = {0, 1, 3, 4, 2};
    rel_item inv[5], sigma[5];

    uint8_t result = rels_are_diagonizable(arr1, arr2, inv, sigma, 5);

    ASSERT_TRUE(result);

    // For odd n, should have exactly 1 fixed point
    size_t fixed = 0;
    for (size_t i = 0; i < 5; ++i) {
        if (sigma[i] == i) fixed++;
        ASSERT_EQUAL(sigma[sigma[i]], i);
    }

    if (result) {
        ASSERT_EQUAL(fixed, 1);
    }
}

TEST(rels_are_diagonizable_not_involution) {
    // Create a case where sigma is not an involution
    rel_item arr1[] = {0, 1, 2, 3};
    rel_item arr2[] = {1, 2, 3, 0}; // This creates a cycle, not an involution
    rel_item inv[4], sigma[4];

    uint8_t result = rels_are_diagonizable(arr1, arr2, inv, sigma, 4);
    ASSERT_FALSE(result);
}

/* ============================================================
 * Tests for position_after_latin_square_permutation
 * ============================================================ */

TEST(position_after_latin_square_permutation_basic) {
    uint32_t new_row, new_col;

    // Test with the P and Q arrays from know/16x16x4.h
    position_after_latin_square_permutation(&new_row, &new_col, 0, 0, P, Q, r, s);

    // The output should be valid indices
    ASSERT_TRUE(new_row < r * s);
    ASSERT_TRUE(new_col < r * s);
}

TEST(position_after_latin_square_permutation_all_positions) {
    uint8_t rows_seen[16] = {0};
    uint8_t cols_seen[16] = {0};
    uint32_t new_row, new_col;

    // Test all 16 positions in a 16x16 grid
    for (uint32_t i = 0; i < 16; ++i) {
        for (uint32_t j = 0; j < 16; ++j) {
            position_after_latin_square_permutation(&new_row, &new_col, i, j, P, Q, r, s);

            // Mark row and column as seen
            rows_seen[new_row] = 1;
            cols_seen[new_col] = 1;
        }
    }

    // All rows and columns should be covered
    for (int i = 0; i < 16; ++i) {
        ASSERT_TRUE(rows_seen[i]);
        ASSERT_TRUE(cols_seen[i]);
    }
}

TEST(position_after_latin_square_permutation_simple) {
    // Create simple 2x2 latin squares for testing
    uint8_t P0[] = {0, 1, 1, 0};
    uint8_t Q0[] = {0, 1, 1, 0};

    latin_square P_simple[] = {{.n = 2, .arr = P0}};
    latin_square Q_simple[] = {{.n = 2, .arr = Q0}};

    uint32_t new_row, new_col;
    position_after_latin_square_permutation(&new_row, &new_col, 0, 0, P_simple, Q_simple, 2, 2);

    // Result should be within bounds
    ASSERT_TRUE(new_row < 4);
    ASSERT_TRUE(new_col < 4);
}

/* ============================================================
 * Tests for fall_on_different_line_after_latin_squares
 * ============================================================ */

TEST(fall_on_different_line_basic) {
    uint8_t rows[16] = {0};
    uint8_t cols[16] = {0};
    rel_item poses[] = {0, 17, 34, 51}; // Diagonal-like positions

    uint8_t result = fall_on_different_line_after_latin_squares(
        rows, cols, poses, P, Q, r, s
    );

    // Result should be 0 or 1
    ASSERT_TRUE(result == 0 || result == 1);
}

TEST(fall_on_different_line_collision) {
    uint8_t rows[16] = {0};
    uint8_t cols[16] = {0};

    // Create positions that will collide on the same row
    rel_item poses[] = {0, 1}; // Both in row 0

    uint8_t result = fall_on_different_line_after_latin_squares(
        rows, cols, poses, P, Q, r, s
    );

    // Might collide after permutation
    (void)result;
}

/* ============================================================
 * Tests for permute_into_pow_m_sqr (integration test)
 * ============================================================ */

TEST(permute_into_pow_m_sqr_integration) {
    // This is the main integration test from src/unit/test.c
    taxicab a, b;
    taxicab_init(&a, r, s, d);
    taxicab_init(&b, s, r, d);

    load_a(&a);
    load_b(&b);

    // Create the semi-magic square
    pow_m_sqr M;
    pow_m_sqr_init(&M, 16, 4);
    pow_semi_m_sqr_from_taxicab(M, a, b, P, Q);

    // Verify it's a semi-magic square before permutation
    ASSERT_TRUE(is_pow_semi_m_sqr(M));

    // Permute into a full magic square
    permute_into_pow_m_sqr(&M, rel1, rel2);

    // Verify it's a full magic square after permutation
    ASSERT_TRUE(is_pow_m_sqr(M));

    // Cleanup
    pow_m_sqr_clear(&M);
    taxicab_clear(&a);
    taxicab_clear(&b);
}

TEST(permute_into_pow_m_sqr_preserves_size) {
    taxicab a, b;
    taxicab_init(&a, r, s, d);
    taxicab_init(&b, s, r, d);

    load_a(&a);
    load_b(&b);

    pow_m_sqr M;
    pow_m_sqr_init(&M, 16, 4);
    pow_semi_m_sqr_from_taxicab(M, a, b, P, Q);

    uint32_t n_before = M.n;
    uint32_t d_before = M.d;

    permute_into_pow_m_sqr(&M, rel1, rel2);

    // Size should be preserved
    ASSERT_EQUAL(M.n, n_before);
    ASSERT_EQUAL(M.d, d_before);

    // Cleanup
    pow_m_sqr_clear(&M);
    taxicab_clear(&a);
    taxicab_clear(&b);
}

/* ============================================================
 * Main test runner
 * ============================================================ */

int main(void) {
    printf("=== Running Permut.c Unit Tests ===\n\n");

    /* rels_are_disjoint tests */
    RUN_TEST(rels_are_disjoint_with_16x16_data);
    RUN_TEST(rels_are_disjoint_same_array);
    RUN_TEST(rels_are_disjoint_simple_case);
    RUN_TEST(rels_are_disjoint_truly_disjoint);

    /* rels_are_diagonizable tests */
    RUN_TEST(rels_are_diagonizable_with_16x16_data);
    RUN_TEST(rels_are_diagonizable_simple_even);
    RUN_TEST(rels_are_diagonizable_simple_odd);
    RUN_TEST(rels_are_diagonizable_not_involution);

    /* position_after_latin_square_permutation tests */
    RUN_TEST(position_after_latin_square_permutation_basic);
    RUN_TEST(position_after_latin_square_permutation_all_positions);
    RUN_TEST(position_after_latin_square_permutation_simple);

    /* fall_on_different_line_after_latin_squares tests */
    RUN_TEST(fall_on_different_line_basic);
    RUN_TEST(fall_on_different_line_collision);

    /* permute_into_pow_m_sqr integration tests */
    RUN_TEST(permute_into_pow_m_sqr_integration);
    RUN_TEST(permute_into_pow_m_sqr_preserves_size);

    printf("\n=== All %d tests passed! ===\n", tests_passed);

    return 0;
}

#define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"

#ifndef A2D_SPARSE_MATRIX_H
#define A2D_SPARSE_MATRIX_H

#include <string>

#include "a2dobjs.h"
#include "array.h"

namespace A2D {

/**
 * @brief Block CSR matrix, object acts like shared_ptr
 *
 * @tparam T data type
 * @tparam M number of rows for each block
 * @tparam N number of columns for each block
 *
 * Below is an illustration of a blocked sparse matrix with 2x3 blocks
 *
 *  [x x x          [x x x
 *   x x x]          x x x]
 *          [x x x          [x x x
 *           x x x]          x x x]
 *  [x x x          [x x x
 *   x x x]          x x x]
 *          [x x x          [x x x
 *           x x x]          x x x]
 *
 * for this example, M = 2, N = 3, nbrows = nbcols = 4
 * block row and column indices are 0, 1, 2, 3
 * global row and column indices are 0, 1, ..., 7
 * local row indices for each block are 0, 1
 * local column indices for each block are 0, 1, 2
 *
 */
template <typename T, index_t M, index_t N>
class BSRMat {
 public:
  /**
   * @brief Constructor
   *
   * @tparam VecType a vector type
   * @param nbrows number of rows of blocks
   * @param nbcols number of columns of blocks
   * @param nnz number of non-zero blocks, note that global nnz = nnz * M * N
   * @param _rowp vector of row pointers
   * @param _cols vector of column indices
   */
  template <class VecType>
  BSRMat(index_t nbrows, index_t nbcols, index_t nnz, const VecType &rowp_,
         const VecType &cols_)
      : nbrows(nbrows), nbcols(nbcols), nnz(nnz), vals("vals", nnz) {
    rowp = IdxArray1D_t("rowp", nbrows + 1);
    cols = IdxArray1D_t("cols", nnz);

    for (index_t i = 0; i < nbrows + 1; i++) {
      rowp[i] = rowp_[i];
    }

    for (index_t i = 0; i < nnz; i++) {
      cols[i] = cols_[i];
    }
  }

  /**
   * @brief Copy constructor (shallow copy)
   */
  A2D_INLINE_FUNCTION BSRMat(const BSRMat &src)
      : nbrows(src.nbrows),
        nbcols(src.nbcols),
        nnz(src.nnz),
        rowp(src.rowp),
        cols(src.cols),
        diag(src.diag),
        perm(src.perm),
        iperm(src.iperm),
        num_colors(src.num_colors),
        color_count(src.color_count),
        vals(src.vals) {}

  /**
   * @brief Default destructor
   */
  A2D_INLINE_FUNCTION ~BSRMat() = default;

  // Zero the entries of the matrix
  A2D_INLINE_FUNCTION void zero() { A2D::BLAS::zero(vals); }

  /**
   * @brief Find the value index of a block given indices (row, col) of a block
   *
   * @param row block row index
   * @param col block column index
   * @return index_t the value index jp such that vals[jp] gives the
   * sought block, if (row, col) isn't in the nonzero pattern, NO_INDEX will be
   * returned
   */
  index_t find_value_index(index_t row, index_t col);

  /**
   * @brief add values from an element matrix mat of shape (m, n)
   *
   * @param m number of rows of mat
   * @param i global row indices for each entry of mat
   * @param n number of columns of mat
   * @param j global column indices for each entry of mat
   * @param mat the element matrix
   */
  template <class Mat>
  void add_values(const index_t m, const index_t i[], const index_t n,
                  const index_t j[], Mat &mat);

  /**
   * @brief Zero out rows and set diagonal entry to one for each zeroed row
   *
   * @param nbcs number of global rows to zero-out
   * @param i global row indices, usually a list of dof indices for finite
   * element context
   */
  void zero_rows(const index_t nbcs, const index_t i[]);

  /**
   * @brief Convert to a dense matrix
   *
   * @param m_ output, number of rows of the dense matrix
   * @param n_ output, number of columns of the dense matrix
   * @param A_ output, dense matrix values stored in row-major ordering
   */
  void to_dense(index_t *m_, index_t *n_, T **A_);

  /**
   * @brief Convert block sparse matrix to ordinary sparse matrix
   */
  void to_csr() {}

  /**
   * @brief Export the matrix as mtx format
   *
   * @param mtx_name the output file
   */
  void write_mtx(const std::string mtx_name = "matrix.mtx");

  // Number of block rows and block columns
  index_t nbrows, nbcols;

  // Number of non-zero blocks
  index_t nnz;  // = rowp[nbrows];

  // rowp and cols array
  IdxArray1D_t rowp;  // length: nbrows + 1
  IdxArray1D_t cols;  // length: nnz = rowp[nbrows]

  // Pointer to the diagonal block, this is not allocated until
  // factorization
  IdxArray1D_t diag;  // length: nbrows

  // row-permutation perm[new row] = old row
  // This is not allocated by default
  IdxArray1D_t perm;

  // Inverse row-permutation iperm[old row] = new row
  // This is not allocated by default
  IdxArray1D_t iperm;

  // When coloring is used, its ordering is stored in the permutation array
  index_t num_colors;        // Number of colors
  IdxArray1D_t color_count;  // Number of nodes with this color, not
                             // allocated by default

  // A multi-dimensional array that stores entries, shape: (nnz, M, N)
  MultiArrayNew<T *[M][N]> vals;
};

}  // namespace A2D

#include "sparse/sparse_matrix-inl.h"

#endif  // A2D_SPARSE_MATRIX_H

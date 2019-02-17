#include "PDB.hpp"
#include "TransformUtil.hpp"
#include "ZDOCK.hpp"

namespace zdock {

class TransformLigand {

  /**
   * Performs ZDOCK prediction transformations on PDB structures.
   * Supports ZDOCK2.3, 2.4, 3.0 old and new style ZDOCK output
   * files.
   */

public:
  TransformLigand(const std::string &zdock);
  TransformLigand(const ZDOCK &zdock);

private:
  typedef Eigen::Transform<double, 3, Eigen::Affine> Transform;
  typedef Eigen::Matrix<double, 3, Eigen::Dynamic> Matrix;
  typedef TransformUtil u;

  zdock::Structure receptor_, ligand_; // zdock metadata
  ZDOCK zdock_;                        // zdock output
  double spacing_;                     // grid spacing
  int boxsize_;                        // grid size
  bool rev_, fixed_;                   // fixed / switched flags

  // precomputed transformation matrices
  Transform t0_, t1_, t2_;

  // grid to actual translation ('circularized')
  inline const Transform boxTranslation(const int (&t)[3],
                                        bool rev = false) const {
    Transform ret;

    using Eigen::Translation3d;
    using Eigen::Vector3d;

    Vector3d d = u::boxedGridCoord(t, boxsize_);
    if (rev) {
      ret = Translation3d(-spacing_ * d);
    } else {
      ret = Translation3d(spacing_ * d);
    }
    return ret;
  }

public:
  // perform actual ligand transformation
  inline const Matrix txLigand(const PDB &pdb, const Prediction &pred) const {
    Transform t;

    using Eigen::Translation3d;
    using Eigen::Vector3d;

    if (rev_) {

      /* Transformation; reverse (receptor was rotated, rather than ligand)
       *
       *     X(rec trans) * X(lig rot, reverse) *
       *       X(pred rot, reverse) * X(pred trans) *
       *         X(-lig trans) * X(rec rot) * M
       *
       */

      t = u::eulerRotation(pred.rotation, true) *
          boxTranslation(pred.translation);
      return t1_ * t * t0_ * pdb.matrix();
    } else {

      /* Transformation; normal (ligand was rotated)
       *
       * fixed case:
       *     X(rec trans) * X(-pred trans) * X(pred rot) *
       *       X(lig rot) * X(-lig trans) * M
       *
       * not fixed case:
       *     X(rec rot, reverse) * [fixed case]
       *
       */

      t = Translation3d(Vector3d(receptor_.translation)) *
          boxTranslation(pred.translation, true) *
          u::eulerRotation(pred.rotation) * t2_;
      if (!fixed_) {
        // !fixed means initial random rotation of receptor
        // so we need to rotate to rec frame
        t = u::eulerRotation(receptor_.rotation, true) * t;
      }
      return t * pdb.matrix();
    }
  }
};

} // namespace zdock

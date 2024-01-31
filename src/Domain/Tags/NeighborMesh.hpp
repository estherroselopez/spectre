// Distributed under the MIT License.
// See LICENSE.txt for details.

#pragma once

#include <boost/functional/hash.hpp>
#include <cstddef>
#include <utility>

#include "DataStructures/DataBox/Tag.hpp"
#include "Domain/Structure/DirectionalIdMap.hpp"
#include "NumericalAlgorithms/Spectral/Mesh.hpp"

namespace domain::Tags {
/*!
 * \brief Holds the mesh of each neighboring element.
 *
 * This knowledge can be used to determine the geometry of mortars between
 * elements. It is kept up to date by AMR.
 *
 * For DG-FD hybrid methods this is necessary to determine what numerical method
 * the neighbor is using. This knowledge can be used for optimizing code.
 */
template <size_t Dim>
struct NeighborMesh : db::SimpleTag {
  using type = DirectionalIdMap<Dim, ::Mesh<Dim>>;
};
}  // namespace domain::Tags

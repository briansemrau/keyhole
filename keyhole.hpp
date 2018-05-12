#ifndef KEYHOLE_HPP
#define KEYHOLE_HPP

#include <clipper.hpp>

using namespace ClipperLib;

namespace Keyhole {

	void keyhole_tree(const PolyNode& node, Paths& out);

}

#endif
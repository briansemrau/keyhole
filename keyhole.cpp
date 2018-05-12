#include "keyhole.hpp"

namespace Keyhole {

	inline cInt perp_dot(const IntPoint& a, const IntPoint& b) {
		return (-a.Y * b.X) + (a.X * b.Y);
	}

	bool edge_collision(const IntPoint& a1, const IntPoint& a2, const IntPoint& b1, const IntPoint& b2, bool relaxed) {
		IntPoint a(a2.X - a1.X, a2.Y - a1.Y);
		IntPoint b(b2.X - b1.X, b2.Y - b1.Y);

		cInt f = perp_dot(a, b);

		// check if lines are parallel
		if (f == 0) {
			// check if collinear intersecting
			if (a2.X - a1.X == 0) {
				cInt u = (b1.Y - a1.Y) / (a2.Y - a1.Y);
				cInt v = (b2.Y - a1.Y) / (a2.Y - a1.Y);
				return u >= 0 && u <= 1 && v >= 0 && v <= 1;
			}
			else {
				cInt u = (b1.X - a1.X) / (a2.X - a1.X);
				cInt v = (b2.X - a1.X) / (a2.X - a1.X);
				return u >= 0 && u <= 1 && v >= 0 && v <= 1;
			}
			// if above algo is bad, then:
			// return perp_dot(b1 - a1, b2 - a1) == 0

			return false;
		}

		IntPoint c(a1.X - b1.X, a1.Y - b1.Y);
		cInt s = perp_dot(a, c);
		cInt t = perp_dot(b, c);

		if (relaxed)
			if (f < 0) {
				if (s >= 0 || t >= 0 || s <= f || t <= f) return false;
			}
			else {
				if (s <= 0 || t <= 0 || s >= f || t >= f) return false;
			}
		else {
			if (f < 0) {
				if (s > 0 || t > 0 || s < f || t < f) return false;
			}
			else {
				if (s < 0 || t < 0 || s > f || t > f) return false;
			}
		}

		return true;
	}

	bool edge_poly_collision(const IntPoint& a1, const IntPoint& a2, const Path& poly) {
		for (Path::const_iterator b2 = poly.begin(), b1 = poly.end() - 1; b2 != poly.end(); b1 = b2, ++b2) {
			bool relaxed = (&a1 == &*b1) || (&a1 == &*b2) || (&a2 == &*b1) || (&a2 == &*b2);
			if (edge_collision(a1, a2, *b1, *b2, relaxed)) return true;
		}
		return false;
	}

	bool splice_vertices(const Path& path1, const Path& path2, Path& result) {
		bool combined = false;
		// Check for vertex intersection
		int n = 0;
		for (const IntPoint& b : path1) {
			for (const IntPoint& a : path2) {

				if (a.X == b.X && a.Y == b.Y) {
					// Splice holes together
					for (const IntPoint& aa : path2) {
						result.push_back(aa);

						// begin splice
						if (&aa == &a) {
							for (int i = (n + 1) % path1.size(); i != n; i = (i + 1) % path1.size()) {
								result.push_back(path1[i]);
							}
							result.push_back(a);
						}
					}

					combined = true;
					break;
				}

			}
			if (combined) break;
			++n;
		}
		return combined;
	}

	Path keyhole_poly(const Path& poly, const Paths& holes) {
		Path keyedpoly = poly;

		// Reconnect holes intersecting at vertices

		Paths combined_holes = holes;
		for (int h1 = 0; h1 < combined_holes.size() - 1; ++h1) {
			const Path& hole1 = combined_holes[h1];
			for (int h2 = h1 + 1; h2 < combined_holes.size(); ++h2) {
				const Path& hole2 = combined_holes[h2];

				Path combined;
				if (splice_vertices(hole1, hole2, combined)) {
					// Add combined hole back into vector
					combined_holes.push_back(combined);

					// Erase both holes that were combined
					combined_holes.erase(combined_holes.begin() + h2);
					combined_holes.erase(combined_holes.begin() + h1);
					--h2;
					--h1;
				}

			}
		}

		// Connect holes to poly intersecting at vertices

		Paths holes_to_key;
		for (const Path& hole : combined_holes) {
			Path combined;
			if (splice_vertices(keyedpoly, hole, combined)) {
				keyedpoly = combined;
			}
			else {
				holes_to_key.push_back(hole);
			}
		}

		// Connect remaining holes to poly using keyholes

		Paths deffered_holes;
		for (const Path& hole : holes_to_key) {
			Path temp_poly;

			bool finished_keyhole = false;

			// Iterate through possible keyholes
			int n = 0;
			for (const IntPoint& b : hole) {
				for (const IntPoint& a : keyedpoly) {

					// Check if valid keyhole
					if (edge_poly_collision(a, b, keyedpoly)) continue;
					bool intersect = false;
					for (const Path& path : holes_to_key) {
						if (edge_poly_collision(a, b, path)) {
							intersect = true;
							break;
						}
					}
					if (intersect) continue;

					// Splice hole into path
					for (const IntPoint& outer : keyedpoly) {
						temp_poly.push_back(outer);

						// begin splice
						if (&outer == &a) {
							temp_poly.push_back(b);
							for (int i = (n + 1) % hole.size(); i != n; i = (i + 1) % hole.size()) {
								temp_poly.push_back(hole[i]);
							}
							temp_poly.push_back(b);
							temp_poly.push_back(a);
						}
					}
					finished_keyhole = true;
					break;
				}
				if (finished_keyhole) break;
				++n;
			}

			if (finished_keyhole)
				keyedpoly = temp_poly;
			else {
				deffered_holes.push_back(hole);
			}
		}

		if (deffered_holes.size() > 0) {
			return keyhole_poly(keyedpoly, deffered_holes);
		}

		return keyedpoly;
	}

	void keyhole_tree(const PolyNode& node, Paths& out) {
		if (node.IsHole()) {
			// Recurse tree
			for (const PolyNode* child : node.Childs)
				keyhole_tree(*child, out);
		}
		else {
			if (node.ChildCount() == 0) {
				// No holes to key, just add poly to output
				out.push_back(node.Contour);
			}
			else {
				// Recurse tree
				// (This can be done before or after keying)
				for (const PolyNode* child : node.Childs)
					keyhole_tree(*child, out);

				// Keyhole the polygon
				Paths holes;
				for (const PolyNode* hole : node.Childs)
					holes.push_back(hole->Contour);

				out.push_back(keyhole_poly(node.Contour, holes));
			}
		}
	}

} // namespace
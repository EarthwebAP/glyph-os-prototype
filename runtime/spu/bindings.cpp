/**
 * Python bindings for SPU merge primitive using pybind11
 *
 * Build: python3 setup.py build_ext --inplace
 * Usage: from spu_merge import merge, Glyph
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "merge_ref.h"

namespace py = pybind11;
using namespace spu;

// Python-friendly Glyph wrapper
struct PyGlyph {
    std::string id;
    std::string content;
    double energy;
    uint32_t activation_count;
    uint64_t last_update_time;
    std::string parent1_id;
    std::string parent2_id;

    // Convert to C++ Glyph
    Glyph to_cpp() const {
        Glyph g;
        strncpy(g.id, id.c_str(), 64);
        strncpy(g.content, content.c_str(), 256);
        g.content_len = std::min(content.length(), size_t(255));
        g.energy = energy;
        g.activation_count = activation_count;
        g.last_update_time = last_update_time;
        return g;
    }

    // Convert from C++ Glyph
    static PyGlyph from_cpp(const Glyph& g) {
        PyGlyph pg;
        pg.id = std::string(g.id);
        pg.content = std::string(g.content, g.content_len);
        pg.energy = g.energy;
        pg.activation_count = g.activation_count;
        pg.last_update_time = g.last_update_time;
        pg.parent1_id = std::string(g.parent1_id);
        pg.parent2_id = std::string(g.parent2_id);
        return pg;
    }
};

// Python-callable merge function
PyGlyph py_merge(const PyGlyph& g1, const PyGlyph& g2) {
    Glyph cpp_g1 = g1.to_cpp();
    Glyph cpp_g2 = g2.to_cpp();
    Glyph result;

    merge(cpp_g1, cpp_g2, result);

    return PyGlyph::from_cpp(result);
}

// Module definition
PYBIND11_MODULE(spu_merge, m) {
    m.doc() = "SPU merge primitive - C++ accelerated glyph merging";

    // Glyph class
    py::class_<PyGlyph>(m, "Glyph")
        .def(py::init<>())
        .def_readwrite("id", &PyGlyph::id)
        .def_readwrite("content", &PyGlyph::content)
        .def_readwrite("energy", &PyGlyph::energy)
        .def_readwrite("activation_count", &PyGlyph::activation_count)
        .def_readwrite("last_update_time", &PyGlyph::last_update_time)
        .def_readwrite("parent1_id", &PyGlyph::parent1_id)
        .def_readwrite("parent2_id", &PyGlyph::parent2_id)
        .def("__repr__", [](const PyGlyph& g) {
            return "<Glyph id='" + g.id.substr(0, 8) + "...' energy=" +
                   std::to_string(g.energy) + ">";
        });

    // merge function
    m.def("merge", &py_merge,
          "Merge two glyphs with energy-based precedence",
          py::arg("glyph1"), py::arg("glyph2"));

    // Version info
    m.attr("__version__") = "1.0.0";
}

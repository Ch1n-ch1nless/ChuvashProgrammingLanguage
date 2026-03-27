#pragma once

namespace parser {

// TODO: think more about it
template <typename T>
concept VisitorConcept = requires(T visitor) { visitor.visit(); };

}  // namespace parser
bool flw_discardPredicate(vec4 color) { return color.a < 0.1; }
void clrwl_computeDiscard(vec4 color) {
 if (flw_discardPredicate(color)) discard;
}

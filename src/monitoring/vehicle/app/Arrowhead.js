// Create a detached group element, which can be added into a parent
// context something like this: d3.select('#some-id').append(Arrowhead());

define( function( require ) {
  'use strict';

  var d3 = require( 'd3' );

  function Arrowhead() {
    return function() {
      var g = d3.select( document.createElementNS( d3.ns.prefix.svg, 'g' ) );

      g.append( 'defs' ).append( 'marker' )
        .attr( {
          'id': 'arrow',
          'viewBox': '0 -5 10 10',
          'refX': 5,
          'refY': 0,
          'markerWidth': 4,
          'markerHeight': 4,
          'orient': 'auto'
        } )
        .append( 'path' )
        .attr( 'd', 'M0,-5L10,0L0,5' );

      return g.node();
    };
  }

  return Arrowhead;
} );


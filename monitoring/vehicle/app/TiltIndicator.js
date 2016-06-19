// Node in scene graph representing vehicle tilt

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );

  // Constructor
  function TiltIndicator() {
    this.drawTiltNode = function( parentSVG, width, height ) {

      // TODO: much drawing code will be common to Compass. Factor out, then reuse here.
      // Heading angle in radians
      var phi = this.heading * Math.PI / 180 - Math.PI / 2;

      // Root SVG element to add as a child to the provided parentSVG
      this.headingNode = parentSVG.append( 'g' )
        .attr( 'transform', 'translate(' + width / 2 + ',' + height / 2 + ')' );

      // Outer radius of polar plot
      var radius = Math.min( width, height ) / 2 - 100;
      this.needleRadius = radius;

      // Linear radial scale function
      var r = d3.scale.linear()
        .domain( [ 0, .5 ] )
        .range( [ 0, radius ] );

    };
  }

  return TiltIndicator;
} );


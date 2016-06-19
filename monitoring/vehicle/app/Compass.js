// Compass module

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );

  // Constructor
  function Compass() {

    // Angle (public) in degrees
    this.heading = 0;

    // Root SVG node to draw in
    this.headingNode = null;

    // Length of compass needle
    this.needleRadius = 0;

    // Update needle direction
    this.update = function( headingDegrees ) {

      this.heading = headingDegrees;

      // Heading angle in radians
      var phi = this.heading * Math.PI / 180 - Math.PI / 2;
      var r = Math.max( this.needleRadius, 10 );
      this.headingNode.select( 'line' ).attr( {
        'x2': 0.9 * r * Math.cos( phi ),
        'y2': 0.9 * r * Math.sin( phi )
      } );
    };

    // Test function - make random direction changes
    this.randomDemo = function() {
      var self = this;
      var heading = 0; // deg
      var steps = 0;
      var sign = 0;
      var loopDelay = 1 / 30; // ms
      function loop() {
        setTimeout( function() {
          self.update( heading );

          // Change direction every once in a while
          if ( steps % Math.round( 400 * Math.random() + 100 ) === 0 ) {
            sign = Math.round( Math.random() ) ? +1 : -1;
          }
          heading += sign * 0.2 * Math.random();
          steps++;
          loop();
        }, loopDelay );
      }
      loop();

    };

    this.drawHeadingNode = function( parentSVG, width, height ) {

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

      // Radial ring(s) - not really necessary here, using only outermost ring.
      // Keeping for possible future use as a speed scale (arrow length could be
      // used as a speed indicator)
      this.headingNode.append( 'g' )
        .attr( 'class', 'r axis' )
        .style( 'fill', 'none' )
        .style( 'stroke', '#777' )
        .selectAll( 'g' )
        .data( r.ticks( 1 ).slice( 1 ) ) // Increase r.ticks for more rings
        .enter().append( 'g' )
        .append( 'circle' )
        .attr( 'r', r );

      // Text labels for degrees
      this.headingNode.append( 'g' )
        .attr( 'class', 'a axis' )
        .selectAll( 'g' )
        .data( d3.range( 0, 360, 30 ) )
        .enter().append( 'g' )
        .attr( 'transform', function( d ) {
          return 'rotate(' + ( d - 90 ) + ')';
        } ).append( 'text' )
        .attr( 'x', radius + 6 )
        .attr( 'dy', '.35em' )
        .style( 'text-anchor', function( d ) {
          return d > 180 ? 'end' : null;
        } )
        .attr( 'transform', function( d ) {
          return d > 180 ? 'rotate(180 ' + ( radius + 6 ) + ',0)' : null;
        } )
        .text( function( d ) {
          return d + '°';
        } );

      // SVG arrowhead
      this.headingNode.append( 'defs' ).append( 'marker' )
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

      // Compass arrow
      this.headingNode.append( 'line' )
        .style( 'stroke', 'black' )
        .style( 'stroke-width', '4px' )
        .attr( {
          'x1': 0,
          'y1': 0,
          'x2': 0.9 * radius * Math.cos( phi ),
          'y2': 0.9 * radius * Math.sin( phi ),
          'marker-end': 'url(#arrow)',
          'class': 'needle'
        } );
    };
  }

  return Compass;
} );


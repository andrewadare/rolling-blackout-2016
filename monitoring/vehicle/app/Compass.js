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
    this.rootNode = null;

    // Length of compass needle
    this.needleRadius = 0;

    this.title = 'Title';

    // Update needle direction
    this.update = function( headingDegrees ) {

      var self = this;

      this.heading = headingDegrees;

      // Heading angle in radians
      var phi = this.heading * Math.PI / 180 - Math.PI/2;
      var r = Math.max( this.needleRadius, 10 );
      this.rootNode.select( '.needle' ).attr( {
        'x2': 0.9 * r * Math.cos( phi ),
        'y2': 0.9 * r * Math.sin( phi )
      } );

      // Update angle readout text
      this.rootNode.select( '.readout' )
        .text( function() {
          return self.title + ': ' + Math.round( headingDegrees ) + '°';
        } );

    };

    this.draw = function( parentSVG, width, height, title ) {

      this.title = title;

      // Heading angle in radians
      var phi = this.heading * Math.PI / 180;

      // Root SVG element to add as a child to the provided parentSVG
      this.rootNode = parentSVG.append( 'g' )
        .attr( 'transform', 'translate(' + width / 2 + ',' + height / 2 + ')' );

      // Outer radius of polar plot
      var radius = Math.min( width, height ) / 3;
      this.needleRadius = radius;

      // Linear radial scale function
      var r = d3.scale.linear()
        .domain( [ 0, .5 ] )
        .range( [ 0, radius ] );

      // Radial ring(s) - not really necessary here, using only outermost ring.
      // Keeping for possible future use as a speed scale (arrow length could be
      // used as a speed indicator)
      this.rootNode.append( 'g' )
        .attr( 'class', 'r axis' )
        .style( 'fill', 'none' )
        .style( 'stroke', '#777' )
        .selectAll( 'g' )
        .data( r.ticks( 1 ).slice( 1 ) ) // Increase r.ticks for more rings
        .enter().append( 'g' )
        .append( 'circle' )
        .attr( 'r', r );

      // Text labels for degrees
      this.rootNode.append( 'g' )
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
      this.rootNode.append( 'defs' ).append( 'marker' )
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
      this.rootNode.append( 'line' )
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

      // Live angle readout
      this.rootNode.append( 'svg:text' )
        .attr( 'class', 'readout' )
        .attr( 'transform', function() {
          return 'translate(' + ( -1.4 * radius ) + ',' + ( -1.3 * radius ) + ')';
        } )
        .text( function() {
          return title + ': ' + '0°';
        } )
        .attr( 'font-size', '20px' );

    };
  }

  return Compass;
} );


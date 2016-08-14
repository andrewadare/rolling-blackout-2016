// Functions for IMU calibration display

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );

  return {

    // From a status like 123, return { a: '0', m: '1', g: '2', s: '3' }
    unpackStatusCodes: function( status ) {
      var s = status.toString();
      var codes = {};
      var keys = [ 'a', 'm', 'g', 's' ];
      var i = 0;
      while ( s.length < 4 ) {
        s = '0' + s;
      }
      keys.forEach( function( k ) {
        codes[ k ] = s.charAt( i );
        i++;
      } );
      return codes;
    },

    updateCalibration: function( data ) {

      function textLine( d ) {
        return d.name + ': ' + d.status;
      }

      d3.select( 'ul' ).selectAll( 'li' )
        .data( data )
        .text( textLine )
        .enter().append( 'li' )
        .attr( 'class', 'cal' )
        .text( textLine );
    }
  };
} );


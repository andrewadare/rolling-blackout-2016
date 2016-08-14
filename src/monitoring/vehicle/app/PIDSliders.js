// Sliders for tuning PID parameters

define( function( require ) {
  'use strict';

  // Module imports
  var d3 = require( 'd3' );

  return {
    add: function( width ) {
      var pidLabels = [ 'kp', 'ki', 'kd' ];
      pidLabels.forEach( function( label ) {
        var p = d3.select( '.row2.col2' ).append( 'p' );
        p.append( 'label' )
          .attr( 'for', label + '-steer' ) // Matches 'input' id attribute
          .html( label + ' = <span id=\"' + label + '-steer-value\"></span>' )
          .style( {
            'display': 'inline-block',
            'width': width + 'px'
          } );
        p.append( 'input' )
          .attr( {
            'type': 'range',
            'min': 0,
            'max': 200,
            'id': label + '-steer'
          } );

      } );


      updateSteerKp( 5 );
      updateSteerKi( 100 );
      updateSteerKd( 1 );

      d3.select( '#kp-steer' ).on( 'input', function() {
        updateSteerKp( +this.value );
      } );
      d3.select( '#ki-steer' ).on( 'input', function() {
        updateSteerKi( +this.value );
      } );
      d3.select( '#kd-steer' ).on( 'input', function() {
        updateSteerKd( +this.value );
      } );

      function updateSteerKp( kp ) {
        d3.select( '#kp-steer-value' ).text( kp );
        d3.select( '#kp-steer' ).property( 'value', kp );
        console.log( 'kp = ', kp );
      }

      function updateSteerKi( ki ) {
        d3.select( '#ki-steer-value' ).text( ki );
        d3.select( '#ki-steer' ).property( 'value', ki );
        console.log( 'ki = ', ki );
      }

      function updateSteerKd( kd ) {
        d3.select( '#kd-steer-value' ).text( kd );
        d3.select( '#kd-steer' ).property( 'value', kd );
        console.log( 'kd = ', kd );
      }
    }


  };
} );


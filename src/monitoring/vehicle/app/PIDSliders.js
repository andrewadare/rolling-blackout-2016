// Sliders for tuning PID parameters

// Example: add sliders to user code; send message when slider value changes
//      PIDSliders.add( panelWidth - 2 );
//
//      // Bind update callbacks to slider values for each slider.
//      [ 'kp', 'ki', 'kd' ].forEach( function( kx ) {
//        d3.select( '#' + kx + '-steer' ).on( 'input', function() {
//          PIDSliders.updateSteerPidCoeff( +this.value, kx );
//
//          ws.send( JSON.stringify( {
//            type: 'update',
//            text: 'steer-' + kx,
//            value: +this.value,
//            id: 0,
//            date: Date.now()
//          } ) );
//        } );
//      } );

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
            'text-align': 'right',
            'width': width / 2 + 'px'
          } );
        p.append( 'input' )
          .attr( {
            'type': 'range',
            'min': 0,
            'max': 200,
            'value': 100,
            'id': label + '-steer'
          } );

      } );
    },

    /**
     * Callback to update slider value
     *
     * @param  {number} coeff - PID coefficient
     * @param  {string} kpid - one of 'kp', 'ki', or 'kd'
     */
    updateSteerPidCoeff: function( coeff, kpid ) {
      d3.select( '#' + kpid + '-steer-value' ).text( coeff );
      d3.select( '#' + kpid + '-steer' ).property( 'value', coeff );
    }
  };
} );


function getSettings(device) {
    // Get current settings
    device.getVariable('get_hour', function(err, data) {
        if (err) {
            console.log('An error occurred:', err);
        } else {
            console.log('Function called succesfully:', data);
            $('#hour').text(data.result);
        }
    });

    device.getVariable('get_minute', function(err, data) {
        if (err) {
            console.log('An error occurred:', err);
        } else {
            console.log('Function called succesfully:', data);
            $('#minute').text(data.result);
        }
    });

    device.getVariable('get_duration', function(err, data) {
        if (err) {
            console.log('An error occurred:', err);
        } else {
            console.log('Function called succesfully:', data);
            $('#duration').text(data.result/60);
        }
    });

    device.getVariable('is_fading', function(err, data) {
        if (err) {
            console.log('An error occurred:', err);
        } else {
            console.log('Function called succesfully:', data);
            if (data.result == 1) {
                $('#fading').text('Yes');
            } else {
                $('#fading').text('No');
            }
        }
    });

    $('#settings').show();
}

function setupActions(device) {
    // Setup action hooks

    $('#start_fading').click(function () {
        device.callFunction('start_fading', null, function(err, data) {
            if (err) {
                console.log('An error occurred:', err);
            } else {
                console.log('Function called succesfully:', data);
                getSettings(device);
            }
        });
    });

    $('#stop_fading').click(function () {
        device.callFunction('stop_fading', null, function(err, data) {
            if (err) {
                console.log('An error occurred:', err);
            } else {
                console.log('Function called succesfully:', data);
                getSettings(device);
            }
        });
    });

    $('#actions').show();
}

$(function () {
    sparkLogin(function(data) {
        // Login feedback
        $('#spark-login').hide();

        console.log(data);

        // Get devices
        var devicesPr = spark.listDevices();

        devicesPr.then(
            function(devices){
                console.log('Devices: ', devices);

                var device = devices[0];

                console.log('Device name: ' + device.name);
                console.log('- connected?: ' + device.connected);
                console.log('- variables: ' + device.variables);
                console.log('- functions: ' + device.functions);
                console.log('- version: ' + device.version);
                console.log('- requires upgrade?: ' + device.requiresUpgrade);

                getSettings(device);
                setupActions();

            },
            function(err) {
                console.log('List devices call failed: ', err);
            }
        );
    });
});


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
            $('#duration').text(data.result/60000);
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

    $('#hour').on('input', _.throttle(function () {
        device.callFunction('set_hour', $('#hour').text(), function(err, data) {
            if (err) {
                console.log('An error occurred:', err);
            } else {
                console.log('Function called succesfully:', data);
                $('#hour').text(data.return_value);
            }
        });
    }, 2000, {leading: false}));

    $('#minute').on('input', _.throttle(function () {
        device.callFunction('set_minute', $('#minute').text(), function(err, data) {
            if (err) {
                console.log('An error occurred:', err);
            } else {
                console.log('Function called succesfully:', data);
                $('#minute').text(data.return_value);
            }
        });
    }, 2000, {leading: false}));

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
                if (data.return_value == -1) {
                    alert('Error starting fade: already fading.');
                }
                getSettings(device);
            }
        });
    });

    $('#reset').click(function () {
        device.callFunction('reset', null, function(err, data) {
            if (err) {
                console.log('An error occurred:', err);
            } else {
                console.log('Function called succesfully:', data);
                getSettings(device);
            }
        });
    });

    $('#update').click(function () {
        getSettings(device);
    });

    $('#actions').show();
}

function setupDevice(device) {
    // Peform setup for device
    getSettings(device);
    setupActions(device);
}

$(function () {
    // Attempt login using stored access token
    var access_token = window.localStorage['access_token'];

    if (access_token != null) {
        console.log('Logging in using access token:', access_token);

        // Weird; in the docs this should be access_token
        spark.login({accessToken: access_token});
    } else {
        // Display login button
        console.log('Rendering login button');

        sparkLogin(function (data) {
            console.log('Logged in:', data);

            // Hide login button
            $('#spark-login').hide();

            // Store access token
            window.localStorage['access_token'] = data.access_token;
        });
    }
});

spark.on('login', function() {
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

            setupDevice(device);
        },
        function(err) {
            console.log('List devices call failed: ', err);
        }
    );
});

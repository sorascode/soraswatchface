module.exports = function(minified) {
    var clayConfig = this;

    function log_hrt() {
        var logHrtToggle = clayConfig.getItemByMessageKey("LOG_HRT");
        logHrtToggle.set(true);
        document.querySelector('button[type="submit"]').click()
    }

    clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
        var hrtTakenButton = clayConfig.getItemById('HRTTaken');
        hrtTakenButton.on('click', log_hrt);

        var logHrtToggle = clayConfig.getItemByMessageKey("LOG_HRT");
        logHrtToggle.set(false)
        logHrtToggle.hide()
    });
};
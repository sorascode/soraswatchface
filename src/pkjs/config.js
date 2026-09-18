module.exports = [
    {
        "type": "heading",
        "defaultValue": "Watchface Settings"
    },
    {
        "type": "text",
        "defaultValue": "Customize your watchface appearance and preferences."
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "Preferences"
            },
            {
                "type": "toggle",
                "messageKey": "TemperatureUnit",
                "label": "Use Fahrenheit",
                "defaultValue": false
            },
            {
                "type": "toggle",
                "messageKey": "ShowDate",
                "label": "Show Date",
                "defaultValue": true
            }
        ]
    },
    {
        "type": "section",
        "items": [
            {
                "type": "heading",
                "defaultValue": "HRT Tracking"
            },
            {
                "type": "toggle",
                "messageKey": "HRTEnabled",
                "label": "Enable HRT Tracking",
                "defaultValue": false
            },
            {
                "type": "select",
                "messageKey": "HRTDay",
                "defaultValue": "mon",
                "label": "HRT Day",
                "options": [
                    {
                        "label": "Sunday",
                        "value": "sun"
                    },
                    {
                        "label": "Monday",
                        "value": "mon"
                    },
                    {
                        "label": "Tuesday",
                        "value": "tue"
                    },
                    {
                        "label": "Wednesday",
                        "value": "wed"
                    },
                    {
                        "label": "Thursday",
                        "value": "thu"
                    },
                    {
                        "label": "Friday",
                        "value": "fri"
                    },
                    {
                        "label": "Saturday",
                        "value": "sat"
                    }
                ]
            },
            {
                "type": "button",
                "id": "HRTTaken",
                "primary": false,
                "defaultValue": "I took my HRT!"
            },
            {
                "type": "toggle",
                "messageKey": "LOG_HRT",
                "label": "SHOULD NOT BE VISIBLE",
                "defaultValue": false
            }
        ]
    },
    {
        "type": "submit",
        "id": "Submit",
        "defaultValue": "Save Settings"
    }
];

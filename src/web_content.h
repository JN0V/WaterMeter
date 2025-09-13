#ifndef WEB_CONTENT_H
#define WEB_CONTENT_H

#include <Arduino.h>

// Store web page HTML in PROGMEM to save RAM
const char COUNTER_PAGE_HTML[] PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Water Meter</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background: #f7f7f7;
        }
        .card {
            max-width: 560px;
            margin: 0 auto;
            background: #fff;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 8px rgba(0,0,0,.1);
        }
        h1 {
            font-size: 20px;
            margin: 0 0 12px;
            color: #333;
        }
        .kv {
            display: flex;
            justify-content: space-between;
            padding: 8px 0;
            border-bottom: 1px solid #eee;
        }
        .kv:last-child {
            border: none;
        }
        form {
            margin-top: 16px;
        }
        input[type=number] {
            width: 100%;
            padding: 10px;
            font-size: 16px;
            margin: 8px 0;
            border: 1px solid #ccc;
            border-radius: 6px;
            box-sizing: border-box;
        }
        button {
            padding: 10px 14px;
            background: #2d7ef7;
            color: #fff;
            border: none;
            border-radius: 6px;
            font-size: 15px;
            cursor: pointer;
        }
        button:hover {
            background: #1a6bd8;
        }
        .row {
            display: flex;
            align-items: center;
            gap: 8px;
            margin: 8px 0;
        }
        label {
            font-weight: bold;
        }
    </style>
</head>
<body>
    <div class="card">
        <h1>Water Meter</h1>
        <div class="kv">
            <span>Total m&sup3;</span>
            <strong>%TOTAL_M3%</strong>
        </div>
        <div class="kv">
            <span>Daily Liters</span>
            <strong>%DAILY_LITERS%</strong>
        </div>
        <div class="kv">
            <span>Yearly m&sup3;</span>
            <strong>%YEARLY_M3%</strong>
        </div>
        <div class="kv">
            <span>Pulses</span>
            <strong>%PULSE_COUNT%</strong>
        </div>
        <form method="POST" action="/counter">
            <label>Set Total Liters</label>
            <input type="number" name="value" step="1" min="0" placeholder="e.g. 12345" required>
            <div class="row">
                <input type="checkbox" id="reset" name="reset_daily">
                <label for="reset">Reset daily counter</label>
            </div>
            <button type="submit">Apply</button>
        </form>
    </div>
</body>
</html>
)";

#endif // WEB_CONTENT_H

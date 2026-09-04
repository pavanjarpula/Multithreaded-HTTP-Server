(function () {
    'use strict';

    var MAX_HISTORY = 60;
    var POLL_INTERVAL = 1500;

    var rpsHistory = [];
    var latencyHistory = [];

    function formatBytes(bytes) {
        if (bytes === 0) return '0 B';
        var units = ['B', 'KB', 'MB', 'GB'];
        var i = Math.floor(Math.log(bytes) / Math.log(1024));
        return (bytes / Math.pow(1024, i)).toFixed(1) + ' ' + units[i];
    }

    function formatUptime(seconds) {
        if (seconds < 60) return seconds + 's';
        if (seconds < 3600) return Math.floor(seconds / 60) + 'm ' + (seconds % 60) + 's';
        var h = Math.floor(seconds / 3600);
        var m = Math.floor((seconds % 3600) / 60);
        return h + 'h ' + m + 'm';
    }

    function updateMetrics(data) {
        document.getElementById('total-requests').textContent = data.total_requests;
        document.getElementById('successful-requests').textContent = data.successful_requests;
        document.getElementById('client-errors').textContent = data.client_errors;
        document.getElementById('server-errors').textContent = data.server_errors;
        document.getElementById('active-connections').textContent = data.active_connections;
        document.getElementById('peak-connections').textContent = data.peak_connections;
        document.getElementById('avg-latency').textContent = data.average_latency_ms.toFixed(2) + ' ms';
        document.getElementById('rps').textContent = data.requests_per_second.toFixed(2);
        document.getElementById('bytes-sent').textContent = formatBytes(data.total_bytes_sent);
        document.getElementById('uptime').textContent = formatUptime(data.uptime_seconds);
        document.getElementById('thread-count').textContent = data.thread_count;
        document.getElementById('queue-size').textContent = data.queue_size;

        var badge = document.getElementById('status-badge');
        badge.textContent = 'ONLINE';
        badge.className = 'badge online';

        rpsHistory.push(data.requests_per_second);
        latencyHistory.push(data.average_latency_ms);

        if (rpsHistory.length > MAX_HISTORY) rpsHistory.shift();
        if (latencyHistory.length > MAX_HISTORY) latencyHistory.shift();

        drawChart('rps-chart', rpsHistory, '#4facfe');
        drawChart('latency-chart', latencyHistory, '#00e676');
    }

    function drawChart(canvasId, data, color) {
        var canvas = document.getElementById(canvasId);
        if (!canvas) return;
        var ctx = canvas.getContext('2d');
        var w = canvas.width;
        var h = canvas.height;

        ctx.clearRect(0, 0, w, h);

        if (data.length < 2) return;

        var maxVal = Math.max.apply(null, data);
        if (maxVal === 0) maxVal = 1;

        var padding = { top: 10, right: 10, bottom: 20, left: 40 };
        var chartW = w - padding.left - padding.right;
        var chartH = h - padding.top - padding.bottom;

        ctx.strokeStyle = '#2a2a3e';
        ctx.lineWidth = 1;
        for (var i = 0; i <= 4; i++) {
            var y = padding.top + (chartH / 4) * i;
            ctx.beginPath();
            ctx.moveTo(padding.left, y);
            ctx.lineTo(w - padding.right, y);
            ctx.stroke();

            ctx.fillStyle = '#555';
            ctx.font = '10px monospace';
            ctx.textAlign = 'right';
            var label = (maxVal - (maxVal / 4) * i).toFixed(1);
            ctx.fillText(label, padding.left - 4, y + 4);
        }

        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.lineWidth = 2;
        ctx.lineJoin = 'round';

        var step = chartW / (MAX_HISTORY - 1);
        var offset = (MAX_HISTORY - data.length) * step;

        for (var j = 0; j < data.length; j++) {
            var x = padding.left + offset + j * step;
            var yPos = padding.top + chartH - (data[j] / maxVal) * chartH;
            if (j === 0) {
                ctx.moveTo(x, yPos);
            } else {
                ctx.lineTo(x, yPos);
            }
        }
        ctx.stroke();

        ctx.beginPath();
        ctx.fillStyle = color + '20';
        ctx.moveTo(padding.left + offset, padding.top + chartH);
        for (var k = 0; k < data.length; k++) {
            var xk = padding.left + offset + k * step;
            var yk = padding.top + chartH - (data[k] / maxVal) * chartH;
            ctx.lineTo(xk, yk);
        }
        ctx.lineTo(padding.left + offset + (data.length - 1) * step, padding.top + chartH);
        ctx.closePath();
        ctx.fill();
    }

    function fetchMetrics() {
        fetch('/metrics')
            .then(function (response) {
                if (!response.ok) throw new Error('HTTP ' + response.status);
                return response.json();
            })
            .then(function (data) {
                updateMetrics(data);
            })
            .catch(function () {
                var badge = document.getElementById('status-badge');
                badge.textContent = 'OFFLINE';
                badge.className = 'badge offline';
            });
    }

    fetchMetrics();
    setInterval(fetchMetrics, POLL_INTERVAL);
})();

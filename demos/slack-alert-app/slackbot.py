import time
import logging
import os
import schedule
import pxapi
from slack.web.client import WebClient
from slack.errors import SlackApiError


# Pixie PxL script.
PXL_SCRIPT = """
import px

df = px.DataFrame(table='http_events', start_time='-5m')

# Filter for px-sock-shop namespace only.
df.namespace = df.ctx['namespace']
df = df[df.namespace == 'px-sock-shop']

# Add column for service name.
df.service = df.ctx['service']

# Add olumn for HTTP response status errors.
df.error = df.http_resp_status >= 400

# Group HTTP events by service, counting errors and total HTTP events.
df = df.groupby(['service']).agg(
    error_count=('error', px.sum),
    total_requests=('http_resp_status', px.count)
)

px.display(df, "status")
"""

logging.basicConfig(level=logging.DEBUG)


def get_pixie_data(api_key, cluster_id):
    # Get data from the Pixie API.

    # Create a Pixie client.
    logging.debug("Authorizing Pixie client.")
    px_client = pxapi.Client(token=api_key)

    # Connect to cluster.
    conn = px_client.connect_to_cluster(cluster_id)
    logging.debug("Pixie client connected to cluster.")

    # Execute the PxL script.
    script = conn.prepare_script(PXL_SCRIPT)
    logging.debug("Pixie cluster executed script.")

    service_stats_msg = ["*Recent 4xx+ Spikes in last 5 minutes:*"]

    # Process table output rows.
    for row in script.results("status"):
        service_stats_msg.append(format_message(row["service"],
                                               row["total_requests"],
                                               row["error_count"]))

    return "\n\n".join(service_stats_msg)


def format_message(service, request_count, error_count):
    # Format Pixie API table row data.
    return (f"`{service}` \t ---> *has {error_count} (>4xx)"
            f" errors out of {request_count} requests.*")


def send_message(slack_client, channel, msg):
    # Send a POST request through the Slack Python client.
    try:
        logging.info(f"Sending {msg!r} to {channel!r}")
        slack_client.chat_postMessage(channel=channel, text=msg)

    except SlackApiError as e:
        logging.error('Request to Slack API Failed: {}.'.format(e.response.status_code))
        logging.error(e.response)


if __name__ == "__main__":

    # Get Pixie API key.
    PIXIE_API_KEY = os.environ['PIXIE_API_KEY']

    # Get Pixie cluster ID.
    PIXIE_CLUSTER_ID = os.environ['PIXIE_CLUSTER_ID']

    # Get data from the Pixie API.
    slack_msg = get_pixie_data(PIXIE_API_KEY, PIXIE_CLUSTER_ID)

    # Get Slackbot access token.
    SLACK_BOT_TOKEN = os.environ['SLACK_BOT_TOKEN']

    # Connect to the Slack client.
    logging.debug("Authorizing Slack client.")
    slack_client = WebClient(SLACK_BOT_TOKEN)

    # Slack channel to post in. Slack App must be a member of this channel.
    SLACK_CHANNEL = "#pixie-alerts"

    # Schedule sending a Slack channel message every 5 minutes.
    schedule.every(5).minutes.do(lambda: send_message(slack_client,
                                                     SLACK_CHANNEL,
                                                     slack_msg))

    logging.info("Message scheduled for %s Slack channel.", SLACK_CHANNEL)

    while True:
        schedule.run_pending()

        # Sleep for 60 seconds between checks on the scheduler.
        time.sleep(60)
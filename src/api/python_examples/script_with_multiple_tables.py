import pxapi


# You'll need to generate an API token.
# For more info, see: https://docs.pixielabs.ai/using-pixie/api-quick-start/
API_TOKEN = "<YOUR_API_TOKEN>"

# PxL script with 2 output tables
PXL_SCRIPT = """
import px
df = px.DataFrame('http_events')[['resp_status','req_path']]
df = df.head(10)
px.display(df, 'http_table')

df2 = px.DataFrame('process_stats')[['upid','cpu_ktime_ns']]
df2 = df2.head(10)
px.display(df2, 'stats')
"""

# create a Pixie client
px_client = pxapi.Client(token=API_TOKEN)

# find first healthy cluster
cluster = px_client.list_healthy_clusters()[0]
print("Selected cluster: ", cluster.name())

# connect to  cluster
conn = px_client.connect_to_cluster(cluster)

# setup the PxL script
script = conn.prepare_script(PXL_SCRIPT)


# create a function to process the table rows
def http_fn(row: pxapi.Row) -> None:
    print(row["resp_status"], row["req_path"])


def stats_fn(row: pxapi.Row) -> None:
    print(row["upid"], row["cpu_ktime_ns"])


# Add a callback function to the script
script.add_callback("http_table", http_fn)
script.add_callback("stats", stats_fn)

# run the script
script.run()


from cfs_test_connection import CfsTest_Connection

class to_lab_test_methods:

    DEFAULT_APP_NAME = "TO_LAB"

    def test_aliveness(self):
        """
        FSW Aliveness Test
        - Send a no-op command
            then verify the command was received (by checking the command counter incremented)
        - Reset the command counter
            then verify the command was received (by checking the command counter was cleared)
        """
        self.connection.print(f"Testing TO_LAB aliveness")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        self.connection.wait_check_packet("HK", 1, 100)

        # Assuming no one else is sending commands, grab the latest command count
        cmd_count = self.connection.tlm("HK", "CommandCounter")

        # Check accepted NOOP command proving application is up and running
        self.connection.cmd("Noop")
        self.connection.wait_check("HK", "CommandCounter", cmd_count + 1, 100)

        # Check accepted Reset Counters command
        self.connection.cmd("ResetCounters")
        self.connection.wait_check("HK", "CommandCounter", 0, 100)


    def test_manage_tlm_subscriptions(self):
        """
        Test Management of Tlm Subscriptions
        - Load the Alternate TO_LAB Subscription Table (reduced telemetry to CFE/TO/CI)
        - Add a new subscription (not in the alt table)
        - Remove the new subscription
        """
        test_table_name = "TO_LAB.Subscriptions"
        test_table_filename = "/cf/to_lab_sub_alt.tbl"
        cfe_tbl_conn = self.connection.spawn_child(app_name="CFE_TBL")

        self.connection.print(f"Testing TO_Lab alt table on {self.connection.get_instance_name()}")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        cfe_tbl_conn.wait_check_packet("HK", 1, 100)

        # Test Table Load command (should take up shared buffer)
        cmd_count = cfe_tbl_conn.tlm("HK", "CommandCounter")
        load_pending_count = cfe_tbl_conn.tlm("HK", "NumLoadPending")
        num_free_shared_bufs = cfe_tbl_conn.tlm("HK", "NumFreeSharedBufs")
        cfe_tbl_conn.cmd("Load", Filename=test_table_filename)
        cfe_tbl_conn.wait_check("HK", "CommandCounter", cmd_count, 100)
        cfe_tbl_conn.wait_check("HK", "NumLoadPending", load_pending_count + 1, 100)
        cfe_tbl_conn.wait_check("HK", "NumFreeSharedBufs", num_free_shared_bufs - 1, 100)

        # Test Table Validate command
        cmd_count = cfe_tbl_conn.tlm("HK", "CommandCounter")
        tbl_validation_count = cfe_tbl_conn.tlm("HK", "ValidationCounter")
        tbl_validation_success_count = cfe_tbl_conn.tlm("HK", "SuccessValCounter")
        tbl_validation_request_count = cfe_tbl_conn.tlm("HK", "NumValRequests")
        cfe_tbl_conn.cmd("Validate", ActiveTableFlag="INACTIVE", TableName=test_table_name)
        cfe_tbl_conn.wait_check("HK", "CommandCounter", cmd_count + 1, 100)
        cfe_tbl_conn.wait_check("HK", "ValidationCounter", tbl_validation_count + 1, 100)
        cfe_tbl_conn.wait_check("HK", "SuccessValCounter", tbl_validation_success_count + 1, 100)
        cfe_tbl_conn.wait_check("HK", "NumValRequests", tbl_validation_request_count + 1, 100)

        # Test Table Activate command
        cmd_count = cfe_tbl_conn.tlm("HK", "CommandCounter")
        load_pending_count = cfe_tbl_conn.tlm("HK", "NumLoadPending")
        num_free_shared_bufs = cfe_tbl_conn.tlm("HK", "NumFreeSharedBufs")
        cfe_tbl_conn.cmd("Activate", TableName=test_table_name)
        cfe_tbl_conn.wait_check("HK", "CommandCounter", cmd_count + 1, 100)
        cfe_tbl_conn.wait_check("HK", "NumLoadPending", load_pending_count - 1, 100)
        cfe_tbl_conn.wait_check("HK", "NumFreeSharedBufs", num_free_shared_bufs + 1, 100)

        # Get a Message ID for test input (must not be included in alt table config)
        fm_conn = self.connection.spawn_child(app_name="FM")
        test_mid = fm_conn.get_tlm_msg_id('HK')

        # Verify that we have a recent TO_LAB HK packet
        self.connection.wait_check_packet("HK", 1, 100)
        cmd_count = self.connection.tlm("HK", "CommandCounter")

        # Check accepted Add Packet command
        self.connection.cmd("AddPacket", StreamValue=test_mid, BufLimit=4)
        self.connection.wait_check("HK", "CommandCounter", cmd_count + 1, 100)

        # Verify that we start receiving the new packet subscription
        fm_conn.wait_check_packet("HK", 1, 100)

        # Check accepted Remove Packet command
        self.connection.cmd("RemovePacket", StreamValue=test_mid)
        self.connection.wait_check("HK", "CommandCounter", cmd_count + 1, 100)


    def test_remove_all_packet_subscriptions(self):
        """
        Test Removing All Packet Subscriptions
        - Then reload the default table to bring telemetry back online
        """
        self.connection.print("Testing TO_LAB Removing All Packet Subscriptions")

        # Verify that we have a recent packet (by waiting for a new one to arrive)
        self.connection.wait_check_packet("HK", 1, 100)

        # Send Remove All command
        # The next few steps have to be done without telemetry confirmation
        self.connection.cmd("RemoveAll")

        # Load, Validate, Activate the default table (to re-enable tlm flow)
        default_table_name = "TO_LAB.Subscriptions"
        default_table_filename = "/cf/to_lab_sub.tbl"
        cfe_tbl_conn = self.connection.spawn_child(app_name="CFE_TBL")
        cfe_tbl_conn.cmd("Load", Filename=default_table_filename)
        cfe_tbl_conn.wait(8)
        cfe_tbl_conn.cmd("Validate", ActiveTableFlag="INACTIVE", TableName=default_table_name)
        cfe_tbl_conn.wait(8)
        cfe_tbl_conn.cmd("Activate", TableName=default_table_name)

        # Verify that we are getting telemetry again
        self.connection.wait_check_packet("HK", 1, 50)
        cfe_tbl_conn.wait_check_packet("HK", 1, 50)


    def setup(self):
        """
        Test Group Setup
        - Runs when Group Setup button is pressed
        - Runs before all scripts when Group Start is pressed
        """
        pass

    def teardown(self):
        """
        Test Group Setup
        - Runs when Group Teardown button is pressed
        - Runs after all scripts when Group Start is pressed
        """
        pass

    def __init__(self, connection : CfsTest_Connection):
        self.connection = connection
